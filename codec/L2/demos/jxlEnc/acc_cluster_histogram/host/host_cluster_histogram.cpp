/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef HOST_CLUSTER_HISTOGRAM_CPP
#define HOST_CLUSTER_HISTOGRAM_CPP

#include <iostream>
#include <sys/time.h>

#include "xcl2.hpp"
#include "xf_utils_sw/logger.hpp"

#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

#define XCL_BANK0 XCL_BANK(0)
#define XCL_BANK1 XCL_BANK(1)
#define XCL_BANK2 XCL_BANK(2)
#define XCL_BANK3 XCL_BANK(3)
#define XCL_BANK4 XCL_BANK(4)
#define XCL_BANK5 XCL_BANK(5)
#define XCL_BANK6 XCL_BANK(6)
#define XCL_BANK7 XCL_BANK(7)
#define XCL_BANK8 XCL_BANK(8)
#define XCL_BANK9 XCL_BANK(9)
#define XCL_BANK10 XCL_BANK(10)
#define XCL_BANK11 XCL_BANK(11)
#define XCL_BANK12 XCL_BANK(12)
#define XCL_BANK13 XCL_BANK(13)
#define XCL_BANK14 XCL_BANK(14)
#define XCL_BANK15 XCL_BANK(15)
#define XCL_BANK16 XCL_BANK(16)
#define XCL_BANK17 XCL_BANK(17)
#define XCL_BANK18 XCL_BANK(18)
#define XCL_BANK19 XCL_BANK(19)
#define XCL_BANK20 XCL_BANK(20)
#define XCL_BANK21 XCL_BANK(21)
#define XCL_BANK22 XCL_BANK(22)
#define XCL_BANK23 XCL_BANK(23)
#define XCL_BANK24 XCL_BANK(24)
#define XCL_BANK25 XCL_BANK(25)
#define XCL_BANK26 XCL_BANK(26)
#define XCL_BANK27 XCL_BANK(27)
#define XCL_BANK28 XCL_BANK(28)
#define XCL_BANK29 XCL_BANK(29)
#define XCL_BANK30 XCL_BANK(30)
#define XCL_BANK31 XCL_BANK(31)
#define XCL_BANK32 XCL_BANK(32)
#define XCL_BANK33 XCL_BANK(33)

unsigned long diff(const struct timeval* newTime, const struct timeval* oldTime) {
    return (newTime->tv_sec - oldTime->tv_sec) * 1000000 + (newTime->tv_usec - oldTime->tv_usec);
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

void hls_ANSclusterHistogram_wrapper(std::string xclbinPath,
                                     uint32_t* config,
                                     //====================
                                     int32_t* histograms0_ptr,
                                     uint32_t* histo_totalcnt0_ptr,
                                     uint32_t* histo_size0_ptr,
                                     uint32_t* nonempty_histo0_ptr,
                                     uint8_t* ctx_map0_ptr,
                                     int32_t* histograms_clusd0_ptr,
                                     uint32_t* histo_size_clusd0_ptr,
                                     int32_t* histograms_clusdin0_ptr,
                                     //====================
                                     int32_t* histograms1_ptr,
                                     uint32_t* histo_totalcnt1_ptr,
                                     uint32_t* histo_size1_ptr,
                                     uint32_t* nonempty_histo1_ptr,
                                     uint8_t* ctx_map1_ptr,
                                     int32_t* histograms_clusd1_ptr,
                                     uint32_t* histo_size_clusd1_ptr,
                                     int32_t* histograms_clusdin1_ptr,
                                     //======================
                                     int32_t* histograms2_ptr,
                                     uint32_t* histo_totalcnt2_ptr,
                                     uint32_t* histo_size2_ptr,
                                     uint32_t* nonempty_histo2_ptr,
                                     uint8_t* ctx_map2_ptr,
                                     int32_t* histograms_clusd2_ptr,
                                     uint32_t* histo_size_clusd2_ptr,
                                     int32_t* histograms_clusdin2_ptr,
                                     //======================
                                     int32_t* histograms3_ptr,
                                     uint32_t* histo_totalcnt3_ptr,
                                     uint32_t* histo_size3_ptr,
                                     uint32_t* nonempty_histo3_ptr,
                                     uint8_t* ctx_map3_ptr,
                                     int32_t* histograms_clusd3_ptr,
                                     uint32_t* histo_size_clusd3_ptr,
                                     int32_t* histograms_clusdin3_ptr,
                                     //======================
                                     int32_t* histograms4_ptr,
                                     uint32_t* histo_totalcnt4_ptr,
                                     uint32_t* histo_size4_ptr,
                                     uint32_t* nonempty_histo4_ptr,
                                     uint8_t* ctx_map4_ptr,
                                     int32_t* histograms_clusd4_ptr,
                                     uint32_t* histo_size_clusd4_ptr,
                                     int32_t* histograms_clusdin4_ptr) {
    printf("[HOST] size= %d\n", config[6]);

    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int fail;

    struct timeval start_time; // End to end time clock start
    gettimeofday(&start_time, 0);

    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device, NULL, NULL, NULL, &fail);
    logger.logCreateContext(fail);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &fail);
    logger.logCreateCommandQueue(fail);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("INFO: Found Device=%s\n", devName.c_str());
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbinPath);

    devices.resize(1);
    cl::Program program(context, devices, xclBins, NULL, &fail);
    logger.logCreateProgram(fail);

    int repInt = 1;
    // create kernels
    std::vector<cl::Kernel> cluster_kernel(repInt);
    for (int i = 0; i < repInt; i++) {
        cluster_kernel[i] = cl::Kernel(program, "JxlEnc_ans_clusterHistogram", &fail);
        logger.logCreateKernel(fail);
    }
    std::cout << "INFO: kernel has been created" << std::endl;

    // declare map of host buffers
    std::cout << "kernel config size:" << 30 << std::endl;
    std::cout << "histogram size: " << config[0] << "," << config[1] << "," << config[2] << "," << config[3] << ","
              << config[4] << std::endl;
    std::cout << "non-empty histogram size: "
              << "," << config[5] << "," << config[6] << "," << config[7] << "," << config[8] << "," << config[9]
              << std::endl;
    std::cout << "largest idx: " << config[10] << "," << config[11] << "," << config[12] << "," << config[13] << ","
              << config[14] << std::endl;
    std::cout << "num cluster: " << config[15] << "," << config[16] << "," << config[17] << "," << config[18] << ","
              << config[19] << std::endl;
    std::cout << "histo_size_clusdin: " << config[20] << "," << config[21] << "," << config[22] << "," << config[23]
              << "," << config[24] << std::endl;
    std::cout << "do_once: " << config[25] << "," << config[26] << "," << config[27] << "," << config[28] << ","
              << config[29] << std::endl;

#define MAX_NUM_CONFIG 30
    uint32_t* hb_config = aligned_alloc<uint32_t>(MAX_NUM_CONFIG);

    int32_t* hb_histograms0_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms1_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms2_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms3_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms4_ptr = aligned_alloc<int32_t>(163840);

    uint32_t* hb_histo_totalcnt0_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_totalcnt1_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_totalcnt2_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_totalcnt3_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_totalcnt4_ptr = aligned_alloc<uint32_t>(4096);

    uint32_t* hb_histo_size0_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_size1_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_size2_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_size3_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histo_size4_ptr = aligned_alloc<uint32_t>(4096);

    uint32_t* hb_nonempty_histo0_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty_histo1_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty_histo2_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty_histo3_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty_histo4_ptr = aligned_alloc<uint32_t>(4096);

    uint8_t* hb_ctx_map0_ptr = aligned_alloc<uint8_t>(4096);
    uint8_t* hb_ctx_map1_ptr = aligned_alloc<uint8_t>(4096);
    uint8_t* hb_ctx_map2_ptr = aligned_alloc<uint8_t>(4096);
    uint8_t* hb_ctx_map3_ptr = aligned_alloc<uint8_t>(4096);
    uint8_t* hb_ctx_map4_ptr = aligned_alloc<uint8_t>(4096);

    int32_t* hb_histograms_clusd0_ptr = aligned_alloc<int32_t>(5120);
    int32_t* hb_histograms_clusd1_ptr = aligned_alloc<int32_t>(5120);
    int32_t* hb_histograms_clusd2_ptr = aligned_alloc<int32_t>(5120);
    int32_t* hb_histograms_clusd3_ptr = aligned_alloc<int32_t>(5120);
    int32_t* hb_histograms_clusd4_ptr = aligned_alloc<int32_t>(5120);

    uint32_t* hb_histo_size_clusd0_ptr = aligned_alloc<uint32_t>(128);
    uint32_t* hb_histo_size_clusd1_ptr = aligned_alloc<uint32_t>(128);
    uint32_t* hb_histo_size_clusd2_ptr = aligned_alloc<uint32_t>(128);
    uint32_t* hb_histo_size_clusd3_ptr = aligned_alloc<uint32_t>(128);
    uint32_t* hb_histo_size_clusd4_ptr = aligned_alloc<uint32_t>(128);

    int32_t* hb_histograms_clusdin0_ptr = aligned_alloc<int32_t>(4096);
    int32_t* hb_histograms_clusdin1_ptr = aligned_alloc<int32_t>(4096);
    int32_t* hb_histograms_clusdin2_ptr = aligned_alloc<int32_t>(4096);
    int32_t* hb_histograms_clusdin3_ptr = aligned_alloc<int32_t>(4096);
    int32_t* hb_histograms_clusdin4_ptr = aligned_alloc<int32_t>(4096);

    for (int j = 0; j < MAX_NUM_CONFIG; j++) {
        hb_config[j] = config[j];
    }

    for (int j = 0; j < 163840; j++) {
        hb_histograms0_ptr[j] = histograms0_ptr[j];
        hb_histograms1_ptr[j] = histograms1_ptr[j];
        hb_histograms2_ptr[j] = histograms2_ptr[j];
        hb_histograms3_ptr[j] = histograms3_ptr[j];
        hb_histograms4_ptr[j] = histograms4_ptr[j];
    }

    for (int j = 0; j < 4096; j++) {
        hb_histo_totalcnt0_ptr[j] = histo_totalcnt0_ptr[j];
        hb_histo_totalcnt1_ptr[j] = histo_totalcnt1_ptr[j];
        hb_histo_totalcnt2_ptr[j] = histo_totalcnt2_ptr[j];
        hb_histo_totalcnt3_ptr[j] = histo_totalcnt3_ptr[j];
        hb_histo_totalcnt4_ptr[j] = histo_totalcnt4_ptr[j];
    }

    for (int j = 0; j < 4096; j++) {
        hb_histo_size0_ptr[j] = histo_size0_ptr[j];
        hb_histo_size1_ptr[j] = histo_size1_ptr[j];
        hb_histo_size2_ptr[j] = histo_size2_ptr[j];
        hb_histo_size3_ptr[j] = histo_size3_ptr[j];
        hb_histo_size4_ptr[j] = histo_size4_ptr[j];
    }

    for (int j = 0; j < 4096; j++) {
        hb_nonempty_histo0_ptr[j] = nonempty_histo0_ptr[j];
        hb_nonempty_histo1_ptr[j] = nonempty_histo1_ptr[j];
        hb_nonempty_histo2_ptr[j] = nonempty_histo2_ptr[j];
        hb_nonempty_histo3_ptr[j] = nonempty_histo3_ptr[j];
        hb_nonempty_histo4_ptr[j] = nonempty_histo4_ptr[j];
    }

    std::vector<cl_mem_ext_ptr_t> mext_o(41);
    mext_o[0] = {XCL_BANK(7), hb_config, 0};

    mext_o[1] = {XCL_BANK(0), hb_histograms0_ptr, 0};
    mext_o[2] = {XCL_BANK(0), hb_histograms1_ptr, 0};
    mext_o[3] = {XCL_BANK(0), hb_histograms2_ptr, 0};
    mext_o[4] = {XCL_BANK(0), hb_histograms3_ptr, 0};
    mext_o[5] = {XCL_BANK(0), hb_histograms4_ptr, 0};

    mext_o[6] = {XCL_BANK(1), hb_histo_totalcnt0_ptr, 0};
    mext_o[7] = {XCL_BANK(1), hb_histo_totalcnt1_ptr, 0};
    mext_o[8] = {XCL_BANK(1), hb_histo_totalcnt2_ptr, 0};
    mext_o[9] = {XCL_BANK(1), hb_histo_totalcnt3_ptr, 0};
    mext_o[10] = {XCL_BANK(1), hb_histo_totalcnt4_ptr, 0};

    mext_o[11] = {XCL_BANK(2), hb_histo_size0_ptr, 0};
    mext_o[12] = {XCL_BANK(2), hb_histo_size1_ptr, 0};
    mext_o[13] = {XCL_BANK(2), hb_histo_size2_ptr, 0};
    mext_o[14] = {XCL_BANK(2), hb_histo_size3_ptr, 0};
    mext_o[15] = {XCL_BANK(2), hb_histo_size4_ptr, 0};

    mext_o[16] = {XCL_BANK(3), hb_nonempty_histo0_ptr, 0};
    mext_o[17] = {XCL_BANK(3), hb_nonempty_histo1_ptr, 0};
    mext_o[18] = {XCL_BANK(3), hb_nonempty_histo2_ptr, 0};
    mext_o[19] = {XCL_BANK(3), hb_nonempty_histo3_ptr, 0};
    mext_o[20] = {XCL_BANK(3), hb_nonempty_histo4_ptr, 0};

    mext_o[21] = {XCL_BANK(4), hb_ctx_map0_ptr, 0};
    mext_o[22] = {XCL_BANK(4), hb_ctx_map1_ptr, 0};
    mext_o[23] = {XCL_BANK(4), hb_ctx_map2_ptr, 0};
    mext_o[24] = {XCL_BANK(4), hb_ctx_map3_ptr, 0};
    mext_o[25] = {XCL_BANK(4), hb_ctx_map4_ptr, 0};

    mext_o[26] = {XCL_BANK(5), hb_histograms_clusd0_ptr, 0};
    mext_o[27] = {XCL_BANK(5), hb_histograms_clusd1_ptr, 0};
    mext_o[28] = {XCL_BANK(5), hb_histograms_clusd2_ptr, 0};
    mext_o[29] = {XCL_BANK(5), hb_histograms_clusd3_ptr, 0};
    mext_o[30] = {XCL_BANK(5), hb_histograms_clusd4_ptr, 0};

    mext_o[31] = {XCL_BANK(6), hb_histo_size_clusd0_ptr, 0};
    mext_o[32] = {XCL_BANK(6), hb_histo_size_clusd1_ptr, 0};
    mext_o[33] = {XCL_BANK(6), hb_histo_size_clusd2_ptr, 0};
    mext_o[34] = {XCL_BANK(6), hb_histo_size_clusd3_ptr, 0};
    mext_o[35] = {XCL_BANK(6), hb_histo_size_clusd4_ptr, 0};

    mext_o[36] = {XCL_BANK(7), hb_histograms_clusdin0_ptr, 0};
    mext_o[37] = {XCL_BANK(7), hb_histograms_clusdin1_ptr, 0};
    mext_o[38] = {XCL_BANK(7), hb_histograms_clusdin2_ptr, 0};
    mext_o[39] = {XCL_BANK(7), hb_histograms_clusdin3_ptr, 0};
    mext_o[40] = {XCL_BANK(7), hb_histograms_clusdin4_ptr, 0};

    // create device buffer and map dev buf to host buf
    cl::Buffer db_config;
    cl::Buffer db_histograms0_ptr;
    cl::Buffer db_histograms1_ptr;
    cl::Buffer db_histograms2_ptr;
    cl::Buffer db_histograms3_ptr;
    cl::Buffer db_histograms4_ptr;
    cl::Buffer db_histo_totalcnt0_ptr;
    cl::Buffer db_histo_totalcnt1_ptr;
    cl::Buffer db_histo_totalcnt2_ptr;
    cl::Buffer db_histo_totalcnt3_ptr;
    cl::Buffer db_histo_totalcnt4_ptr;
    cl::Buffer db_histo_size0_ptr;
    cl::Buffer db_histo_size1_ptr;
    cl::Buffer db_histo_size2_ptr;
    cl::Buffer db_histo_size3_ptr;
    cl::Buffer db_histo_size4_ptr;
    cl::Buffer db_nonempty_histo0_ptr;
    cl::Buffer db_nonempty_histo1_ptr;
    cl::Buffer db_nonempty_histo2_ptr;
    cl::Buffer db_nonempty_histo3_ptr;
    cl::Buffer db_nonempty_histo4_ptr;
    cl::Buffer db_ctx_map0_ptr;
    cl::Buffer db_ctx_map1_ptr;
    cl::Buffer db_ctx_map2_ptr;
    cl::Buffer db_ctx_map3_ptr;
    cl::Buffer db_ctx_map4_ptr;
    cl::Buffer db_histograms_clusd0_ptr;
    cl::Buffer db_histograms_clusd1_ptr;
    cl::Buffer db_histograms_clusd2_ptr;
    cl::Buffer db_histograms_clusd3_ptr;
    cl::Buffer db_histograms_clusd4_ptr;
    cl::Buffer db_histo_size_clusd0_ptr;
    cl::Buffer db_histo_size_clusd1_ptr;
    cl::Buffer db_histo_size_clusd2_ptr;
    cl::Buffer db_histo_size_clusd3_ptr;
    cl::Buffer db_histo_size_clusd4_ptr;
    cl::Buffer db_histograms_clusdin0_ptr;
    cl::Buffer db_histograms_clusdin1_ptr;
    cl::Buffer db_histograms_clusdin2_ptr;
    cl::Buffer db_histograms_clusdin3_ptr;
    cl::Buffer db_histograms_clusdin4_ptr;

    db_config = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(uint32_t) * 30, &mext_o[0]);

    db_histograms0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[1]);
    db_histograms1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[2]);
    db_histograms2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[3]);
    db_histograms3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[4]);
    db_histograms4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[5]);

    db_histo_totalcnt0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[6]);
    db_histo_totalcnt1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[7]);
    db_histo_totalcnt2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[8]);
    db_histo_totalcnt3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[9]);
    db_histo_totalcnt4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[10]);

    db_histo_size0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(uint32_t) * 4096, &mext_o[11]);
    db_histo_size1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(uint32_t) * 4096, &mext_o[12]);
    db_histo_size2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(uint32_t) * 4096, &mext_o[13]);
    db_histo_size3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(uint32_t) * 4096, &mext_o[14]);
    db_histo_size4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(uint32_t) * 4096, &mext_o[15]);

    db_nonempty_histo0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[16]);
    db_nonempty_histo1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[17]);
    db_nonempty_histo2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[18]);
    db_nonempty_histo3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[19]);
    db_nonempty_histo4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(uint32_t) * 4096, &mext_o[20]);

    db_ctx_map0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                 sizeof(uint8_t) * 4096, &mext_o[21]);
    db_ctx_map1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                 sizeof(uint8_t) * 4096, &mext_o[22]);
    db_ctx_map2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                 sizeof(uint8_t) * 4096, &mext_o[23]);
    db_ctx_map3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                 sizeof(uint8_t) * 4096, &mext_o[24]);
    db_ctx_map4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                 sizeof(uint8_t) * 4096, &mext_o[25]);

    db_histograms_clusd0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(int32_t) * 5120, &mext_o[26]);
    db_histograms_clusd1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(int32_t) * 5120, &mext_o[27]);
    db_histograms_clusd2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(int32_t) * 5120, &mext_o[28]);
    db_histograms_clusd3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(int32_t) * 5120, &mext_o[29]);
    db_histograms_clusd4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(int32_t) * 5120, &mext_o[30]);

    db_histo_size_clusd0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(uint32_t) * 128, &mext_o[31]);
    db_histo_size_clusd1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(uint32_t) * 128, &mext_o[32]);
    db_histo_size_clusd2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(uint32_t) * 128, &mext_o[33]);
    db_histo_size_clusd3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(uint32_t) * 128, &mext_o[34]);
    db_histo_size_clusd4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                          sizeof(uint32_t) * 128, &mext_o[35]);

    db_histograms_clusdin0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            sizeof(int32_t) * 4096, &mext_o[36]);
    db_histograms_clusdin1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            sizeof(int32_t) * 4096, &mext_o[37]);
    db_histograms_clusdin2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            sizeof(int32_t) * 4096, &mext_o[38]);
    db_histograms_clusdin3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            sizeof(int32_t) * 4096, &mext_o[39]);
    db_histograms_clusdin4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                            sizeof(int32_t) * 4096, &mext_o[40]);

    // add buffers to migrate
    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;

    ob_in.push_back(db_config);
    ob_in.push_back(db_histograms0_ptr);
    ob_in.push_back(db_histograms1_ptr);
    ob_in.push_back(db_histograms2_ptr);
    ob_in.push_back(db_histograms3_ptr);
    ob_in.push_back(db_histograms4_ptr);
    ob_in.push_back(db_histo_totalcnt0_ptr);
    ob_in.push_back(db_histo_totalcnt1_ptr);
    ob_in.push_back(db_histo_totalcnt2_ptr);
    ob_in.push_back(db_histo_totalcnt3_ptr);
    ob_in.push_back(db_histo_totalcnt4_ptr);
    ob_in.push_back(db_histo_size0_ptr);
    ob_in.push_back(db_histo_size1_ptr);
    ob_in.push_back(db_histo_size2_ptr);
    ob_in.push_back(db_histo_size3_ptr);
    ob_in.push_back(db_histo_size4_ptr);
    ob_in.push_back(db_nonempty_histo0_ptr);
    ob_in.push_back(db_nonempty_histo1_ptr);
    ob_in.push_back(db_nonempty_histo2_ptr);
    ob_in.push_back(db_nonempty_histo3_ptr);
    ob_in.push_back(db_nonempty_histo4_ptr);

    ob_out.push_back(db_config);
    ob_out.push_back(db_ctx_map0_ptr);
    ob_out.push_back(db_ctx_map1_ptr);
    ob_out.push_back(db_ctx_map2_ptr);
    ob_out.push_back(db_ctx_map3_ptr);
    ob_out.push_back(db_ctx_map4_ptr);
    ob_out.push_back(db_histograms_clusd0_ptr);
    ob_out.push_back(db_histograms_clusd1_ptr);
    ob_out.push_back(db_histograms_clusd2_ptr);
    ob_out.push_back(db_histograms_clusd3_ptr);
    ob_out.push_back(db_histograms_clusd4_ptr);
    ob_out.push_back(db_histo_size_clusd0_ptr);
    ob_out.push_back(db_histo_size_clusd1_ptr);
    ob_out.push_back(db_histo_size_clusd2_ptr);
    ob_out.push_back(db_histo_size_clusd3_ptr);
    ob_out.push_back(db_histo_size_clusd4_ptr);
    ob_out.push_back(db_histograms_clusdin0_ptr);
    ob_out.push_back(db_histograms_clusdin1_ptr);
    ob_out.push_back(db_histograms_clusdin2_ptr);
    ob_out.push_back(db_histograms_clusdin3_ptr);
    ob_out.push_back(db_histograms_clusdin4_ptr);

    // set kernel args
    for (int i = 0; i < repInt; i++) {
        cluster_kernel[i].setArg(0, db_config);
        cluster_kernel[i].setArg(1, db_histograms0_ptr);
        cluster_kernel[i].setArg(2, db_histo_totalcnt0_ptr);
        cluster_kernel[i].setArg(3, db_histo_size0_ptr);
        cluster_kernel[i].setArg(4, db_nonempty_histo0_ptr);
        cluster_kernel[i].setArg(5, db_ctx_map0_ptr);
        cluster_kernel[i].setArg(6, db_histograms_clusd0_ptr);
        cluster_kernel[i].setArg(7, db_histo_size_clusd0_ptr);
        cluster_kernel[i].setArg(8, db_histograms_clusdin0_ptr);
        cluster_kernel[i].setArg(9, db_histograms1_ptr);
        cluster_kernel[i].setArg(10, db_histo_totalcnt1_ptr);
        cluster_kernel[i].setArg(11, db_histo_size1_ptr);
        cluster_kernel[i].setArg(12, db_nonempty_histo1_ptr);
        cluster_kernel[i].setArg(13, db_ctx_map1_ptr);
        cluster_kernel[i].setArg(14, db_histograms_clusd1_ptr);
        cluster_kernel[i].setArg(15, db_histo_size_clusd1_ptr);
        cluster_kernel[i].setArg(16, db_histograms_clusdin1_ptr);
        cluster_kernel[i].setArg(17, db_histograms2_ptr);
        cluster_kernel[i].setArg(18, db_histo_totalcnt2_ptr);
        cluster_kernel[i].setArg(19, db_histo_size2_ptr);
        cluster_kernel[i].setArg(20, db_nonempty_histo2_ptr);
        cluster_kernel[i].setArg(21, db_ctx_map2_ptr);
        cluster_kernel[i].setArg(22, db_histograms_clusd2_ptr);
        cluster_kernel[i].setArg(23, db_histo_size_clusd2_ptr);
        cluster_kernel[i].setArg(24, db_histograms_clusdin2_ptr);
        cluster_kernel[i].setArg(25, db_histograms3_ptr);
        cluster_kernel[i].setArg(26, db_histo_totalcnt3_ptr);
        cluster_kernel[i].setArg(27, db_histo_size3_ptr);
        cluster_kernel[i].setArg(28, db_nonempty_histo3_ptr);
        cluster_kernel[i].setArg(29, db_ctx_map3_ptr);
        cluster_kernel[i].setArg(30, db_histograms_clusd3_ptr);
        cluster_kernel[i].setArg(31, db_histo_size_clusd3_ptr);
        cluster_kernel[i].setArg(32, db_histograms_clusdin3_ptr);
        cluster_kernel[i].setArg(33, db_histograms4_ptr);
        cluster_kernel[i].setArg(34, db_histo_totalcnt4_ptr);
        cluster_kernel[i].setArg(35, db_histo_size4_ptr);
        cluster_kernel[i].setArg(36, db_nonempty_histo4_ptr);
        cluster_kernel[i].setArg(37, db_ctx_map4_ptr);
        cluster_kernel[i].setArg(38, db_histograms_clusd4_ptr);
        cluster_kernel[i].setArg(39, db_histo_size_clusd4_ptr);
        cluster_kernel[i].setArg(40, db_histograms_clusdin4_ptr);
    }

    // launch kernel and calculate kernel execution time
    std::cout << "INFO: Kernel Start" << std::endl;
    // declare events
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    // migrate
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);
    q.enqueueTask(cluster_kernel[0], &events_write, &events_kernel[0]);
    q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
    q.finish();

    struct timeval end_time;
    gettimeofday(&end_time, 0);
    std::cout << "INFO: Finish kernel execution" << std::endl;
    std::cout << "INFO: Finish E2E execution" << std::endl;

    // print related times
    unsigned long timeStart, timeEnd, exec_time0;
    std::cout << "-------------------------------------------------------" << std::endl;
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
    exec_time0 = (timeEnd - timeStart) / 1000.0;
    std::cout << "INFO: Data transfer from host to device: " << exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
    exec_time0 = (timeEnd - timeStart) / 1000.0;
    std::cout << "INFO: Kernel1 Data transfer from device to host: " << exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    exec_time0 = 0;
    for (int i = 0; i < 1; ++i) {
        events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
        events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
        exec_time0 += (timeEnd - timeStart) / 1000.0;

        std::cout << "INFO: Kernel" << i + 1 << " execution: " << (timeEnd - timeStart) / 1000.0 << " us\n";
        std::cout << "-------------------------------------------------------" << std::endl;
    }
    std::cout << "INFO: kernel total execution: " << exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    unsigned long exec_timeE2E = diff(&end_time, &start_time);
    std::cout << "INFO: FPGA execution time:" << exec_timeE2E << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    for (int j = 0; j < MAX_NUM_CONFIG; j++) {
        config[j] = hb_config[j];
    }
    // std::cout << "out kernel config size:" << 30 << std::endl;
    // std::cout << "histogram size: " << config[0] << "," << config[1] << "," << config[2]
    //    << "," << config[3] << "," << config[4] << std::endl;
    // std::cout << "non-empty histogram size: " << "," << config[5] << "," << config[6]
    //    << "," << config[7] << "," << config[8] << "," << config[9] << std::endl;
    // std::cout << "largest idx: " << config[10] << "," << config[11] << "," << config[12]
    //    << "," << config[13] << "," << config[14] << std::endl;
    // std::cout << "num cluster: " << config[15] << "," << config[16] << "," << config[17]
    //    << "," << config[18] << "," << config[19] << std::endl;
    // std::cout << "histo_size_clusdin: " << config[20] << "," << config[21] << "," << config[22]
    //    << "," << config[23] << "," << config[24] << std::endl;
    // std::cout << "do_once: " << config[25] << "," << config[26] << "," << config[27]
    //    << "," << config[28] << "," << config[29] << std::endl;

    // output
    std::cout << "ctx_map_ptr:" << std::endl;
    for (int j = 0; j < 4096; j++) {
        ctx_map0_ptr[j] = hb_ctx_map0_ptr[j];
        ctx_map1_ptr[j] = hb_ctx_map1_ptr[j];
        ctx_map2_ptr[j] = hb_ctx_map2_ptr[j];
        ctx_map3_ptr[j] = hb_ctx_map3_ptr[j];
        ctx_map4_ptr[j] = hb_ctx_map4_ptr[j];
    }

    std::cout << "histograms_clusd_ptr:" << std::endl;
    for (int j = 0; j < 5120; j++) {
        histograms_clusd0_ptr[j] = hb_histograms_clusd0_ptr[j];
        histograms_clusd1_ptr[j] = hb_histograms_clusd1_ptr[j];
        histograms_clusd2_ptr[j] = hb_histograms_clusd2_ptr[j];
        histograms_clusd3_ptr[j] = hb_histograms_clusd3_ptr[j];
        histograms_clusd4_ptr[j] = hb_histograms_clusd4_ptr[j];
    }

    std::cout << "histo_size_clusd_ptr:" << std::endl;
    for (int j = 0; j < 128; j++) {
        histo_size_clusd0_ptr[j] = hb_histo_size_clusd0_ptr[j];
        histo_size_clusd1_ptr[j] = hb_histo_size_clusd1_ptr[j];
        histo_size_clusd2_ptr[j] = hb_histo_size_clusd2_ptr[j];
        histo_size_clusd3_ptr[j] = hb_histo_size_clusd3_ptr[j];
        histo_size_clusd4_ptr[j] = hb_histo_size_clusd4_ptr[j];
    }

    std::cout << "histograms_clusdin_ptr:" << std::endl;
    for (int j = 0; j < 4096; j++) {
        histograms_clusdin0_ptr[j] = hb_histograms_clusdin0_ptr[j];
        histograms_clusdin1_ptr[j] = hb_histograms_clusdin1_ptr[j];
        histograms_clusdin2_ptr[j] = hb_histograms_clusdin2_ptr[j];
        histograms_clusdin3_ptr[j] = hb_histograms_clusdin3_ptr[j];
        histograms_clusdin4_ptr[j] = hb_histograms_clusdin4_ptr[j];
    }

    // for(int i=0; i<config[17]; i++) {
    //    for(int j=0; j<histo_size_clusd2_ptr[i]; j++) {
    //        printf("[HOST] cluster 2 %d %d %d\n", i, j, histograms_clusd2_ptr[i*40+j]);
    //    }
    //}
    // for(int j=0; j<config[22]; j++) {
    //    printf("[HOST] cluster in 2 %d %d\n", j, histograms_clusdin2_ptr[j]);
    //}

    free(hb_config);
    free(hb_histograms0_ptr);
    free(hb_histograms1_ptr);
    free(hb_histograms2_ptr);
    free(hb_histograms3_ptr);
    free(hb_histograms4_ptr);
    free(hb_histo_totalcnt0_ptr);
    free(hb_histo_totalcnt1_ptr);
    free(hb_histo_totalcnt2_ptr);
    free(hb_histo_totalcnt3_ptr);
    free(hb_histo_totalcnt4_ptr);
    free(hb_histo_size0_ptr);
    free(hb_histo_size1_ptr);
    free(hb_histo_size2_ptr);
    free(hb_histo_size3_ptr);
    free(hb_histo_size4_ptr);
    free(hb_nonempty_histo0_ptr);
    free(hb_nonempty_histo1_ptr);
    free(hb_nonempty_histo2_ptr);
    free(hb_nonempty_histo3_ptr);
    free(hb_nonempty_histo4_ptr);
    free(hb_ctx_map0_ptr);
    free(hb_ctx_map1_ptr);
    free(hb_ctx_map2_ptr);
    free(hb_ctx_map3_ptr);
    free(hb_ctx_map4_ptr);
    free(hb_histograms_clusd0_ptr);
    free(hb_histograms_clusd1_ptr);
    free(hb_histograms_clusd2_ptr);
    free(hb_histograms_clusd3_ptr);
    free(hb_histograms_clusd4_ptr);
    free(hb_histo_size_clusd0_ptr);
    free(hb_histo_size_clusd1_ptr);
    free(hb_histo_size_clusd2_ptr);
    free(hb_histo_size_clusd3_ptr);
    free(hb_histo_size_clusd4_ptr);
    free(hb_histograms_clusdin0_ptr);
    free(hb_histograms_clusdin1_ptr);
    free(hb_histograms_clusdin2_ptr);
    free(hb_histograms_clusdin3_ptr);
    free(hb_histograms_clusdin4_ptr);
    std::cout << "finished opencl host" << std::endl;
}

#endif
