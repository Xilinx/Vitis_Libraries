/*
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
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

#include <iostream>
#include <new>
#include <cstdlib>
#include <cstdint>

#include <sys/time.h>

#include "xf_utils_sw/logger.hpp"
#include "xf_utils_sw/arg_parser.hpp"

#include "xcl2.hpp"
#include "config.hpp" // define NUM_PORTS, S_WIDTH

#include "ap_int.h"

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

int main(int argc, const char* argv[]) {
    using namespace xf::common::utils_sw;
    Logger logger;
    ArgParser parser(argc, argv);
    parser.addOption("", "--xclbin", "xclbin path", "", true);
    std::string xclbin_path = parser.getAs<std::string>("xclbin");

    std::cout << "Starting test.\n";
    //
    uint64_t D0 = 10;
    uint64_t D1 = 10;
    uint64_t D2 = 10;
    uint64_t D3 = 10;

    unsigned int descriptor_num[NUM_PORTS];
    unsigned int descriptor_size[NUM_PORTS];
    unsigned int buff_size[NUM_PORTS];
    uint64_t* cfg_in[NUM_PORTS];
    uint64_t* cfg_out[NUM_PORTS];
    ap_uint<S_WIDTH>* in_buff[NUM_PORTS];
    ap_uint<S_WIDTH>* out_buff[NUM_PORTS];
    unsigned int seed = 12;
    srand(seed);
    for (int i = 0; i < NUM_PORTS; i++) {
        descriptor_num[i] = 4;
        descriptor_size[i] = descriptor_num[i] * 9 + 1; // 9 word per descriptor
        buff_size[i] = D0 * D1 * D2 * D3 * descriptor_num[i];

        cfg_in[i] = aligned_alloc<uint64_t>(descriptor_size[i]);
        cfg_in[i][0] = descriptor_num[i];
        for (unsigned j = 0; j < cfg_in[i][0]; j++) {
            // 6 [10x10x10x10] cuboids, compacted store
            cfg_in[i][1 + j * 9 + 0] = D0 * D1 * D2 * D3 * j;
            cfg_in[i][1 + j * 9 + 1] = 1;
            cfg_in[i][1 + j * 9 + 2] = D0;
            cfg_in[i][1 + j * 9 + 3] = D0;
            cfg_in[i][1 + j * 9 + 4] = D1;
            cfg_in[i][1 + j * 9 + 5] = D0 * D1;
            cfg_in[i][1 + j * 9 + 6] = D2;
            cfg_in[i][1 + j * 9 + 7] = D0 * D1 * D2;
            cfg_in[i][1 + j * 9 + 8] = D3;
        }
        cfg_out[i] = aligned_alloc<uint64_t>(descriptor_size[i]);
        cfg_out[i][0] = descriptor_num[i];
        for (unsigned j = 0; j < cfg_out[i][0]; j++) {
            cfg_out[i][1 + j * 9 + 0] = D0 * D1 * D2 * D3 * (cfg_in[i][0] - 1 - j);
            cfg_out[i][1 + j * 9 + 1] = 1;
            cfg_out[i][1 + j * 9 + 2] = D0;
            cfg_out[i][1 + j * 9 + 3] = D0;
            cfg_out[i][1 + j * 9 + 4] = D1;
            cfg_out[i][1 + j * 9 + 5] = D0 * D1;
            cfg_out[i][1 + j * 9 + 6] = D2;
            cfg_out[i][1 + j * 9 + 7] = D0 * D1 * D2;
            cfg_out[i][1 + j * 9 + 8] = D3;
        }

        in_buff[i] = aligned_alloc<ap_uint<S_WIDTH> >(buff_size[i]);
        out_buff[i] = aligned_alloc<ap_uint<S_WIDTH> >(buff_size[i]);
        for (unsigned j = 0; j < buff_size[i]; j++) {
            in_buff[i][j] = j * 100 + i;
            out_buff[i][j] = 0;
        }
    }
    std::cout << "Host map buffer has been allocated and set.\n";

    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    // Create context and command queue for selected device
    cl_int err;
    cl::Context context(device, nullptr, nullptr, nullptr, &err);
    logger.logCreateContext(err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    logger.logCreateCommandQueue(err);

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins, nullptr, &err);
    logger.logCreateProgram(err);

    cl::Kernel kernel0(program, "mm2s_4d", &err);
    logger.logCreateKernel(err);
    cl::Kernel kernel1(program, "s2mm_4d", &err);
    logger.logCreateKernel(err);
    std::cout << "Kernel loaded" << std::endl;

    cl_mem_ext_ptr_t mext_read[2 * NUM_PORTS];  // (desc, mm), (desc, mm), ...
    cl_mem_ext_ptr_t mext_write[2 * NUM_PORTS]; // (desc, mm), (desc, mm), ...
    for (unsigned n = 0; n < NUM_PORTS; ++n) {
        mext_read[0 + 2 * n] = {2 + 3 * n, cfg_in[n], kernel0()};
        mext_read[1 + 2 * n] = {0 + 3 * n, in_buff[n], kernel0()};

        mext_write[0 + 2 * n] = {2 + 3 * n, cfg_out[n], kernel1()};
        mext_write[1 + 2 * n] = {1 + 3 * n, out_buff[n], kernel1()};
    }
    std::cout << "mext done" << std::endl;

    cl::Buffer read_buf[2 * NUM_PORTS];  // (desc, mm), (desc, mm), ...
    cl::Buffer write_buf[2 * NUM_PORTS]; // (desc, mm), (desc, mm), ...
    for (unsigned n = 0; n < NUM_PORTS; ++n) {
        read_buf[0 + 2 * n] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         (sizeof(uint64_t) * descriptor_size[n]), &mext_read[0 + 2 * n]);
        read_buf[1 + 2 * n] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         (sizeof(ap_uint<S_WIDTH>) * buff_size[n]), &mext_read[1 + 2 * n]);

        write_buf[0 + 2 * n] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          (sizeof(uint64_t) * descriptor_size[n]), &mext_write[0 + 2 * n]);
        write_buf[1 + 2 * n] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          (sizeof(ap_uint<S_WIDTH>) * buff_size[n]), &mext_write[1 + 2 * n]);
    }

    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);

    std::vector<std::vector<cl::Event> > write_events(1);
    std::vector<std::vector<cl::Event> > kernel_events(1);
    std::vector<std::vector<cl::Event> > read_events(1);
    write_events[0].resize(1);
    kernel_events[0].resize(2);
    read_events[0].resize(1);

    // write data to DDR
    std::vector<cl::Memory> ib;
    for (unsigned n = 0; n < 2 * NUM_PORTS; n++) {
        ib.push_back(read_buf[n]);
        ib.push_back(write_buf[n]);
    }

    q.enqueueMigrateMemObjects(ib, 0, nullptr, &write_events[0][0]);

    // set args and enqueue kernel
    for (unsigned n = 0; n < NUM_PORTS; ++n) {
        kernel0.setArg(0 + 3 * n, read_buf[1 + 2 * n]); // mm
        kernel0.setArg(2 + 3 * n, read_buf[0 + 2 * n]); // desc

        kernel1.setArg(1 + 3 * n, write_buf[1 + 2 * n]); // mm
        kernel1.setArg(2 + 3 * n, write_buf[0 + 2 * n]); // desc
    }

    q.enqueueTask(kernel0, &write_events[0], &kernel_events[0][0]);
    q.enqueueTask(kernel1, &write_events[0], &kernel_events[0][1]);

    // read data from DDR
    std::vector<cl::Memory> ob;
    for (unsigned n = 0; n < NUM_PORTS; ++n) {
        ob.push_back(write_buf[1 + 2 * n]);
    }
    q.enqueueMigrateMemObjects(ob, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[0], &read_events[0][0]);

    // wait all to finish
    q.flush();
    q.finish();
    gettimeofday(&end_time, 0);
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) << "us" << std::endl;

    // check result
    int nerror = 0;
    for (int i = 0; i < NUM_PORTS; i++) {
        for (unsigned j = 0; j < cfg_out[i][0]; j++) {
            for (unsigned k = 0; k < D0 * D1 * D2 * D3; k++) {
                if (out_buff[i][j * D0 * D1 * D2 * D3 + k] !=
                    in_buff[i][(cfg_out[i][0] - 1 - j) * D0 * D1 * D2 * D3 + k]) {
                    nerror++;
                }
            }
        }
        if (!nerror) {
            std::cout << "Out buffer " << i << " passes check" << std::endl;
        } else {
            std::cout << "TEST FAIL\n"
                      << "Error found in out buffer " << i << std::endl;
            break;
        }
    }
    if (!nerror) std::cout << "TEST PASS" << std::endl;

    return nerror;
}
