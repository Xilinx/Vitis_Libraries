/*
 * Copyright 2020 Xilinx, Inc.
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

#include "xf_database/gqe_platform.hpp"

namespace xf {
namespace database {
namespace gqe {

using namespace std;

PlatformInit::PlatformInit(string shell_name) {
    device_shell_name = shell_name;

    cl_int err;

    // get plaform numbers
    cl_uint platform_count = 0;
    err = clGetPlatformIDs(0, NULL, &platform_count);

    // only scan the first 16 platforms
    if (platform_count > 16) {
        platform_count = 16;
        cout << "Only the first 16 platforms will be scanned" << endl;
    }

    // find Xilinx platform
    cl_platform_id platforms[16] = {0};
    err = clGetPlatformIDs(platform_count, platforms, NULL);

    char platform_name[256];
    bool founded = false;
    for (cl_int pl_idx = 0; pl_idx < platform_count; ++pl_idx) {
        err = clGetPlatformInfo(platforms[pl_idx], CL_PLATFORM_NAME, 256, platform_name, 0);
        if (strcmp(platform_name, "Xilinx") != 0) {
            continue;
        } else {
            xilinx_platform_id = platforms[pl_idx];
            founded = true;
            break;
        }
    }

    // find desired device based on shell_name
    if (!founded) {
        cout << "ERROR: Xilinx platform is not founded" << endl;
        device_num = 0;
    } else {
        cl_uint dv_num;
        clGetDeviceIDs(xilinx_platform_id, CL_DEVICE_TYPE_ACCELERATOR, 0, NULL, &dv_num);

        if (dv_num > 16) {
            dv_num = 16;
            cout << "Only the first 16 device of Xilinx platform will be scanned" << endl;
        }

        cl_device_id dv[16];
        err = clGetDeviceIDs(xilinx_platform_id, CL_DEVICE_TYPE_ACCELERATOR, dv_num, dv, NULL);

        for (cl_uint i = 0; i < dv_num; i++) {
            char device_name[256];
            clGetDeviceInfo(dv[i], CL_DEVICE_NAME, 256, device_name, 0);
            if (strcmp(device_name, device_shell_name.c_str()) == 0) {
                device_id.push_back(dv[i]);
            }
        }
        device_num = device_id.size();
    }
    cout << "PlatformInit done" << std::endl;
}

void PlatformInit::print() {
    cout << "\nXilinx platform's ID: " << xilinx_platform_id << endl;
    cout << "Shell name: " << device_shell_name << endl;
    cout << "Device Num: " << device_num << endl;
    cout << "Devices: " << endl;
    for (int i = 0; i < device_num; i++) {
        char name[256];
        clGetDeviceInfo(device_id[i], CL_DEVICE_NAME, 256, name, 0);
        cout << "device[" << i << "]'s name = " << name << " ID = " << device_id[i] << endl;
    }
}

void PlatformInit::release() {
    for (auto itr = device_id.begin(); itr != device_id.end(); ++itr) {
        clReleaseDevice(*itr);
    }
}

} // gqe
} // database
} // xf
