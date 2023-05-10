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
#include <ap_int.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <new>
#include <cstdlib>
#include <xcl2.hpp>
#include <cstdint>

#include "xf_utils_sw/logger.hpp"
#include "xf_utils_sw/arg_parser.hpp"

#define WIDTH 64

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
    ap_uint<64> D0 = 10;
    ap_uint<64> D1 = 10;
    ap_uint<64> D2 = 10;
    ap_uint<64> D3 = 10;

    unsigned int descriptor_num[2] = {4, 6};
    unsigned int descriptor_size[2];
    unsigned int buff_size[2];
    ap_uint<64>* cfg_in[2];
    ap_uint<64>* cfg_out[2];
    ap_uint<WIDTH>* in_buff[2];
    ap_uint<WIDTH>* out_buff[2];
    unsigned int seed = 12;
    srand(seed);
    for (int i = 0; i < 2; i++) {
        descriptor_size[i] = descriptor_num[i] * 9 + 1;
        buff_size[i] = D0 * D1 * D2 * D3 * descriptor_num[i];

        cfg_in[i] = aligned_alloc<ap_uint<64> >(descriptor_size[i]);
        cfg_in[i][0] = descriptor_num[i];
        for (int j = 0; j < cfg_in[i][0]; j++) {
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
        cfg_out[i] = aligned_alloc<ap_uint<64> >(descriptor_size[i]);
        cfg_out[i][0] = descriptor_num[i];
        for (int j = 0; j < cfg_out[i][0]; j++) {
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

        in_buff[i] = aligned_alloc<ap_uint<WIDTH> >(buff_size[i]);
        for (int j = 0; j < buff_size[i]; j++) {
            in_buff[i][j] = j * 10 + i;
        }

        out_buff[i] = aligned_alloc<ap_uint<WIDTH> >(buff_size[i]);
        memset(out_buff[i], 0, buff_size[i] * sizeof(ap_uint<WIDTH>));
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

    cl::Kernel kernel0(program, "cuboid_read", &err);
    logger.logCreateKernel(err);
    cl::Kernel kernel1(program, "cuboid_write", &err);
    logger.logCreateKernel(err);
    std::cout << "Kernel loaded" << std::endl;

    cl_mem_ext_ptr_t mext_read[4];
    mext_read[0] = {0, cfg_in[0], kernel0()};
    mext_read[1] = {1, in_buff[0], kernel0()};
    mext_read[2] = {3, cfg_in[1], kernel0()};
    mext_read[3] = {4, in_buff[1], kernel0()};
    cl_mem_ext_ptr_t mext_write[4];
    mext_write[0] = {0, cfg_out[0], kernel1()};
    mext_write[1] = {2, out_buff[0], kernel1()};
    mext_write[2] = {3, cfg_out[1], kernel1()};
    mext_write[3] = {5, out_buff[1], kernel1()};
    std::cout << "mext done" << std::endl;

    cl::Buffer read_buf[4];
    read_buf[0] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             (size_t)(sizeof(ap_uint<64>) * descriptor_size[0]), &mext_read[0]);
    read_buf[1] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             (size_t)(sizeof(ap_uint<WIDTH>) * buff_size[0]), &mext_read[1]);
    read_buf[2] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             (size_t)(sizeof(ap_uint<64>) * descriptor_size[1]), &mext_read[2]);
    read_buf[3] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             (size_t)(sizeof(ap_uint<WIDTH>) * buff_size[1]), &mext_read[3]);
    cl::Buffer write_buf[4];
    write_buf[0] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                              (size_t)(sizeof(ap_uint<64>) * descriptor_size[0]), &mext_write[0]);
    write_buf[1] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                              (size_t)(sizeof(ap_uint<WIDTH>) * buff_size[0]), &mext_write[1]);
    write_buf[2] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                              (size_t)(sizeof(ap_uint<64>) * descriptor_size[1]), &mext_write[2]);
    write_buf[3] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                              (size_t)(sizeof(ap_uint<WIDTH>) * buff_size[1]), &mext_write[3]);

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
    for (int i = 0; i < 4; i++) {
        ib.push_back(read_buf[i]);
        ib.push_back(write_buf[i]);
    }

    q.enqueueMigrateMemObjects(ib, 0, nullptr, &write_events[0][0]);

    // set args and enqueue kernel
    kernel0.setArg(0, read_buf[0]);
    kernel0.setArg(1, read_buf[1]);
    kernel0.setArg(3, read_buf[2]);
    kernel0.setArg(4, read_buf[3]);

    kernel1.setArg(0, write_buf[0]);
    kernel1.setArg(2, write_buf[1]);
    kernel1.setArg(3, write_buf[2]);
    kernel1.setArg(5, write_buf[3]);

    q.enqueueTask(kernel0, &write_events[0], &kernel_events[0][0]);
    q.enqueueTask(kernel1, &write_events[0], &kernel_events[0][1]);

    // read data from DDR
    std::vector<cl::Memory> ob;
    ob.push_back(write_buf[1]);
    ob.push_back(write_buf[3]);
    q.enqueueMigrateMemObjects(ob, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[0], &read_events[0][0]);

    // wait all to finish
    q.flush();
    q.finish();
    gettimeofday(&end_time, 0);
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) << "us" << std::endl;

    // check result
    int nerror[2] = {0, 0};
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < cfg_out[i][0]; j++) {
            for (int k = 0; k < D0 * D1 * D2 * D3; k++) {
                if (out_buff[i][j * D0 * D1 * D2 * D3 + k] !=
                    in_buff[i][(cfg_out[i][0] - 1 - j) * D0 * D1 * D2 * D3 + k]) {
                    nerror[i]++;
                }
            }
        }

        /*
        for (int j = 0; j < buff_size[i]; j++) {
            if (out_buff[i][j] != in_buff[i][j]) {
                nerror[i]++;
                std::cout << "out_buff[" << i << "][" << j << "] = " << out_buff[i][j] << "  in_buff[" << i << "][" << j
                          << "] = " << in_buff[i][j] << std::endl;
            }
        }
        */
    }

    return (nerror[0] + nerror[1]);
}
