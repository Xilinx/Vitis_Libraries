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

#ifndef HOST_LOSSY_ENC_COMPUTE_SC_CPP
#define HOST_LOSSY_ENC_COMPUTE_SC_CPP

#include <iostream>
#include <sys/time.h>
#include "ap_int.h"

#include "host_lossy_enc_compute.hpp"

#ifndef HLS_TEST
#include "xf_utils_sw/logger.hpp"
#endif

unsigned long diff(const struct timeval* newTime, const struct timeval* oldTime) {
    return (newTime->tv_sec - oldTime->tv_sec) * 1000000 + (newTime->tv_usec - oldTime->tv_usec);
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

void hls_lossy_enc_compute_wrapper(std::string xclbinPath,      // xclbin
                                   int config[32],              // mm15, input
                                   float config_fl[32],         // mm16, input
                                   float* hls_opsin_1,          // mm1, input
                                   float* hls_opsin_2,          // mm2, input
                                   float* hls_opsin_3,          // mm3, input
                                   float* hls_quant_field,      // mm4, input
                                   float* hls_masking_field,    // mm5, input
                                   float* aq_map_f,             // mm6, input
                                   int8_t* cmap_axi,            // mm7, output
                                   int* ac_coef_axiout,         // mm8, output
                                   unsigned char* strategy_all, // mm9, output
                                   int* raw_quant_field_i,      // mm10, output
                                   uint32_t* hls_order,         // mm11, output
                                   float* hls_dc8x8,            // mm12, output
                                   float* hls_dc16x16,          // mm13, output
                                   float* hls_dc32x32           // mm14, output
                                   ) {
#ifndef HLS_TEST

    auto config_pool = lossy_acc::create_bufpool(vpp::input);
    auto config_fl_pool = lossy_acc::create_bufpool(vpp::input);
    auto hls_opsin_1_pool = lossy_acc::create_bufpool(vpp::input);
    auto hls_opsin_2_pool = lossy_acc::create_bufpool(vpp::input);
    auto hls_opsin_3_pool = lossy_acc::create_bufpool(vpp::input);
    auto quant_field_row_pool = lossy_acc::create_bufpool(vpp::input);
    auto masking_field_row_pool = lossy_acc::create_bufpool(vpp::input);
    auto aq_map_f_pool = lossy_acc::create_bufpool(vpp::input);
    auto cmap_axi_pool = lossy_acc::create_bufpool(vpp::output);
    auto ac_coef_axiout_pool = lossy_acc::create_bufpool(vpp::output);
    auto strategy_all_pool = lossy_acc::create_bufpool(vpp::output);
    auto raw_quant_field_i_pool = lossy_acc::create_bufpool(vpp::output);
    auto hls_order_pool = lossy_acc::create_bufpool(vpp::output);
    auto hls_dc8x8_pool = lossy_acc::create_bufpool(vpp::output);
    auto hls_dc16x16_pool = lossy_acc::create_bufpool(vpp::output);
    auto hls_dc32x32_pool = lossy_acc::create_bufpool(vpp::output);

    lossy_acc::send_while([&]() -> bool {
        int* acc_config = (int*)lossy_acc::alloc_buf(config_pool, sizeof(int) * MAX_NUM_CONFIG);
        float* acc_config_fl = (float*)lossy_acc::alloc_buf(config_fl_pool, sizeof(float) * MAX_NUM_CONFIG);
        float* acc_hls_opsin_1 = (float*)lossy_acc::alloc_buf(hls_opsin_1_pool, sizeof(float) * ALL_PIXEL);
        float* acc_hls_opsin_2 = (float*)lossy_acc::alloc_buf(hls_opsin_2_pool, sizeof(float) * ALL_PIXEL);
        float* acc_hls_opsin_3 = (float*)lossy_acc::alloc_buf(hls_opsin_3_pool, sizeof(float) * ALL_PIXEL);
        float* acc_quant_field_row =
            (float*)lossy_acc::alloc_buf(quant_field_row_pool, sizeof(float) * BLOCK8_H * BLOCK8_W);
        float* acc_masking_field_row =
            (float*)lossy_acc::alloc_buf(masking_field_row_pool, sizeof(float) * BLOCK8_H * BLOCK8_W);
        float* acc_aq_map_f = (float*)lossy_acc::alloc_buf(aq_map_f_pool, sizeof(float) * BLOCK8_H * BLOCK8_W);
        int8_t* acc_cmap_axi = (int8_t*)lossy_acc::alloc_buf(cmap_axi_pool, sizeof(int8_t) * TILE_W * TILE_H * 2);
        int* acc_ac_coef_axiout = (int*)lossy_acc::alloc_buf(ac_coef_axiout_pool, sizeof(int) * ALL_PIXEL);
        unsigned char* acc_strategy_all =
            (unsigned char*)lossy_acc::alloc_buf(strategy_all_pool, sizeof(float) * BLOCK8_H * BLOCK8_W);
        int* acc_raw_quant_field_i =
            (int*)lossy_acc::alloc_buf(raw_quant_field_i_pool, sizeof(int) * BLOCK8_H * BLOCK8_W);
        uint32_t* acc_hls_order = (uint32_t*)lossy_acc::alloc_buf(hls_order_pool, sizeof(uint32_t) * MAX_ORDER);
        float* acc_hls_dc8x8 = (float*)lossy_acc::alloc_buf(hls_dc8x8_pool, sizeof(float) * ALL_PIXEL);
        float* acc_hls_dc16x16 = (float*)lossy_acc::alloc_buf(hls_dc16x16_pool, sizeof(float) * ALL_PIXEL);
        float* acc_hls_dc32x32 = (float*)lossy_acc::alloc_buf(hls_dc32x32_pool, sizeof(float) * ALL_PIXEL);

        memcpy(acc_config, config, sizeof(int) * MAX_NUM_CONFIG);
        memcpy(acc_config_fl, config_fl, sizeof(float) * MAX_NUM_CONFIG);
        memcpy(acc_hls_opsin_1, hls_opsin_1, sizeof(float) * ALL_PIXEL);
        memcpy(acc_hls_opsin_2, hls_opsin_2, sizeof(float) * ALL_PIXEL);
        memcpy(acc_hls_opsin_3, hls_opsin_3, sizeof(float) * ALL_PIXEL);
        memcpy(acc_quant_field_row, hls_quant_field, sizeof(float) * BLOCK8_H * BLOCK8_W);
        memcpy(acc_masking_field_row, hls_masking_field, sizeof(float) * BLOCK8_H * BLOCK8_W);
        memcpy(acc_aq_map_f, aq_map_f, sizeof(float) * BLOCK8_H * BLOCK8_W);
        // memcpy(acc_cmap_axi, cmap_axi, sizeof(int8_t) * TILE_W * TILE_H * 2);
        // memcpy(acc_ac_coef_axiout, ac_coef_axiout, sizeof(int) * ALL_PIXEL);
        // memcpy(acc_strategy_all, strategy_all, sizeof(unsigned char) * BLOCK8_H * BLOCK8_W);
        // memcpy(acc_raw_quant_field_i, raw_quant_field_i, sizeof(int) * BLOCK8_H * BLOCK8_W);
        // memcpy(acc_hls_order, hls_order, sizeof(uint32_t) * MAX_ORDER);
        // memcpy(acc_hls_dc8x8, hls_dc8x8, sizeof(float) * ALL_PIXEL);
        // memcpy(acc_hls_dc16x16, hls_dc16x16, sizeof(float) * ALL_PIXEL);
        // memcpy(acc_hls_dc32x32, hls_dc32x32, sizeof(float) * ALL_PIXEL);

        lossy_acc::compute(acc_config, acc_config_fl, acc_hls_opsin_1, acc_hls_opsin_2, acc_hls_opsin_3,
                           acc_quant_field_row, acc_masking_field_row, acc_aq_map_f, acc_cmap_axi, acc_ac_coef_axiout,
                           acc_strategy_all, acc_raw_quant_field_i, acc_hls_order, acc_hls_dc8x8, acc_hls_dc16x16,
                           acc_hls_dc32x32);
        return 0;
    });

    lossy_acc::receive_all_in_order([&]() {
        int* acc_config = (int*)lossy_acc::get_buf(config_pool);
        float* acc_config_fl = (float*)lossy_acc::get_buf(config_fl_pool);
        float* acc_hls_opsin_1 = (float*)lossy_acc::get_buf(hls_opsin_1_pool);
        float* acc_hls_opsin_2 = (float*)lossy_acc::get_buf(hls_opsin_2_pool);
        float* acc_hls_opsin_3 = (float*)lossy_acc::get_buf(hls_opsin_3_pool);
        float* acc_quant_field_row = (float*)lossy_acc::get_buf(quant_field_row_pool);
        float* acc_masking_field_row = (float*)lossy_acc::get_buf(masking_field_row_pool);
        float* acc_aq_map_f = (float*)lossy_acc::get_buf(aq_map_f_pool);
        int8_t* acc_cmap_axi = (int8_t*)lossy_acc::get_buf(cmap_axi_pool);
        int* acc_ac_coef_axiout = (int*)lossy_acc::get_buf(ac_coef_axiout_pool);
        unsigned char* acc_strategy_all = (unsigned char*)lossy_acc::get_buf(strategy_all_pool);
        int* acc_raw_quant_field_i = (int*)lossy_acc::get_buf(raw_quant_field_i_pool);
        uint32_t* acc_hls_order = (uint32_t*)lossy_acc::get_buf(hls_order_pool);
        float* acc_hls_dc8x8 = (float*)lossy_acc::get_buf(hls_dc8x8_pool);
        float* acc_hls_dc16x16 = (float*)lossy_acc::get_buf(hls_dc16x16_pool);
        float* acc_hls_dc32x32 = (float*)lossy_acc::get_buf(hls_dc32x32_pool);

        // memcpy(config, acc_config, sizeof(int) * MAX_NUM_CONFIG);
        // memcpy(config_fl, acc_config_fl, sizeof(float) * MAX_NUM_CONFIG);
        // memcpy(hls_opsin_1, acc_hls_opsin_1, sizeof(float) * ALL_PIXEL);
        // memcpy(hls_opsin_2, acc_hls_opsin_2, sizeof(float) * ALL_PIXEL);
        // memcpy(hls_opsin_3, acc_hls_opsin_3, sizeof(float) * ALL_PIXEL);
        // memcpy(hls_quant_field, acc_quant_field_row, sizeof(float) * BLOCK8_H * BLOCK8_W);
        // memcpy(hls_masking_field, acc_masking_field_row, sizeof(float) * BLOCK8_H * BLOCK8_W);
        // memcpy(aq_map_f, acc_aq_map_f, sizeof(float) * BLOCK8_H * BLOCK8_W);
        memcpy(cmap_axi, acc_cmap_axi, sizeof(int8_t) * TILE_W * TILE_H * 2);
        memcpy(ac_coef_axiout, acc_ac_coef_axiout, sizeof(int) * ALL_PIXEL);
        memcpy(strategy_all, acc_strategy_all, sizeof(unsigned char) * BLOCK8_H * BLOCK8_W);
        memcpy(raw_quant_field_i, acc_raw_quant_field_i, sizeof(int) * BLOCK8_H * BLOCK8_W);
        memcpy(hls_order, acc_hls_order, sizeof(uint32_t) * MAX_ORDER);
        memcpy(hls_dc8x8, acc_hls_dc8x8, sizeof(float) * ALL_PIXEL);
        memcpy(hls_dc16x16, acc_hls_dc16x16, sizeof(float) * ALL_PIXEL);
        memcpy(hls_dc32x32, acc_hls_dc32x32, sizeof(float) * ALL_PIXEL);
    });

    lossy_acc::join();

// xf::common::utils_sw::Logger logger(std::cout, std::cerr);
// cl_int fail;

// struct timeval start_time; // End to end time clock start
// gettimeofday(&start_time, 0);

// // platform related operations
// std::vector<cl::Device> devices = xcl::get_xil_devices();
// cl::Device device = devices[0];

// // Creating Context and Command Queue for selected Device
// cl::Context context(device, NULL, NULL, NULL, &fail);
// logger.logCreateContext(fail);
// cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &fail);
// logger.logCreateCommandQueue(fail);
// std::string devName = device.getInfo<CL_DEVICE_NAME>();
// printf("INFO: Found Device=%s\n", devName.c_str());
// cl::Program::Binaries xclBins = xcl::import_binary_file(xclbinPath);

// devices.resize(1);
// cl::Program program(context, devices, xclBins, NULL, &fail);
// logger.logCreateProgram(fail);

// int repInt = 1;
// // create kernels
// std::vector<cl::Kernel> hls_lossy_enc_compute(repInt);
// for (int i = 0; i < repInt; i++) {
//     hls_lossy_enc_compute[i] = cl::Kernel(program, "hls_lossy_enc_compute", &fail);
//     logger.logCreateKernel(fail);
// }
// std::cout << "INFO: kernel has been created" << std::endl;

// // 1. create all I/O Buffer
// float* hb_hls_opsin_1 = aligned_alloc<float>(ALL_PIXEL);
// float* hb_hls_opsin_2 = aligned_alloc<float>(ALL_PIXEL);
// float* hb_hls_opsin_3 = aligned_alloc<float>(ALL_PIXEL);
// float* hb_hls_quant_field = aligned_alloc<float>(BLOCK8_H * BLOCK8_W);
// float* hb_hls_masking_field = aligned_alloc<float>(BLOCK8_H * BLOCK8_W);
// float* hb_aq_map_f = aligned_alloc<float>(BLOCK8_H * BLOCK8_W);
// int8_t* hb_cmap_axi = aligned_alloc<int8_t>(TILE_W * TILE_H * 2);
// int32_t* hb_ac_coef_axiout = aligned_alloc<int32_t>(ALL_PIXEL);
// unsigned char* hb_strategy_all = aligned_alloc<u_char>(BLOCK8_W * BLOCK8_H);
// int32_t* hb_raw_quant_field_i = aligned_alloc<int32_t>(BLOCK8_H * BLOCK8_W);
// uint32_t* hb_hls_order = aligned_alloc<uint32_t>(MAX_ORDER);
// float* hb_hls_dc8x8 = aligned_alloc<float>(ALL_PIXEL);
// float* hb_hls_dc16x16 = aligned_alloc<float>(ALL_PIXEL);
// float* hb_hls_dc32x32 = aligned_alloc<float>(ALL_PIXEL);
// int32_t* hb_config = aligned_alloc<int32_t>(MAX_NUM_CONFIG);
// float* hb_config_fl = aligned_alloc<float>(MAX_NUM_CONFIG);

// //==================================================
// // 2. init all the host Buffers
// //==================================================

// // input port
// for (int j = 0; j < MAX_NUM_CONFIG; j++) {
//     hb_config[j] = config[j];
// }

// for (int j = 0; j < MAX_NUM_CONFIG; j++) {
//     hb_config_fl[j] = config_fl[j];
// }

// for (int j = 0; j < ALL_PIXEL; j++) {
//     hb_hls_opsin_1[j] = hls_opsin_1[j];
// }

// for (int j = 0; j < ALL_PIXEL; j++) {
//     hb_hls_opsin_2[j] = hls_opsin_2[j];
// }

// for (int j = 0; j < ALL_PIXEL; j++) {
//     hb_hls_opsin_3[j] = hls_opsin_3[j];
// }

// for (int j = 0; j < BLOCK8_H * BLOCK8_W; j++) {
//     hb_hls_quant_field[j] = hls_quant_field[j];
// }

// for (int j = 0; j < BLOCK8_H * BLOCK8_W; j++) {
//     hb_hls_masking_field[j] = hls_masking_field[j];
// }

// for (int j = 0; j < BLOCK8_H * BLOCK8_W; j++) {
//     hb_aq_map_f[j] = aq_map_f[j];
// }

// // mapping to HBM banks
// std::vector<cl_mem_ext_ptr_t> mext_o(33);
// mext_o[0] = {(((unsigned int)(0)) | XCL_MEM_TOPOLOGY), hb_hls_opsin_1, 0};
// mext_o[1] = {(((unsigned int)(1)) | XCL_MEM_TOPOLOGY), hb_hls_opsin_2, 0};
// mext_o[2] = {(((unsigned int)(2)) | XCL_MEM_TOPOLOGY), hb_hls_opsin_3, 0};
// mext_o[3] = {(((unsigned int)(3)) | XCL_MEM_TOPOLOGY), hb_hls_quant_field, 0};
// mext_o[4] = {(((unsigned int)(4)) | XCL_MEM_TOPOLOGY), hb_hls_masking_field, 0};
// mext_o[5] = {(((unsigned int)(5)) | XCL_MEM_TOPOLOGY), hb_aq_map_f, 0};
// mext_o[6] = {(((unsigned int)(6)) | XCL_MEM_TOPOLOGY), hb_cmap_axi, 0};
// mext_o[7] = {(((unsigned int)(7)) | XCL_MEM_TOPOLOGY), hb_ac_coef_axiout, 0};
// mext_o[8] = {(((unsigned int)(8)) | XCL_MEM_TOPOLOGY), hb_strategy_all, 0};
// mext_o[9] = {(((unsigned int)(9)) | XCL_MEM_TOPOLOGY), hb_raw_quant_field_i, 0};
// mext_o[10] = {(((unsigned int)(10)) | XCL_MEM_TOPOLOGY), hb_hls_order, 0};
// mext_o[11] = {(((unsigned int)(11)) | XCL_MEM_TOPOLOGY), hb_hls_dc8x8, 0};
// mext_o[12] = {(((unsigned int)(12)) | XCL_MEM_TOPOLOGY), hb_hls_dc16x16, 0};
// mext_o[13] = {(((unsigned int)(13)) | XCL_MEM_TOPOLOGY), hb_hls_dc32x32, 0};
// mext_o[14] = {(((unsigned int)(14)) | XCL_MEM_TOPOLOGY), hb_config, 0};
// mext_o[15] = {(((unsigned int)(15)) | XCL_MEM_TOPOLOGY), hb_config_fl, 0};

// //===================================================
// // 3. create device Buffer and map dev buf to host buf,
// //===================================================
// cl::Buffer db_hls_opsin_1;       // mm1, input
// cl::Buffer db_hls_opsin_2;       // mm2, input
// cl::Buffer db_hls_opsin_3;       // mm3, input
// cl::Buffer db_hls_quant_field;   // mm4, input
// cl::Buffer db_hls_masking_field; // mm5, input
// cl::Buffer db_aq_map_f;          // mm6, input
// cl::Buffer db_cmap_axi;          // mm7, output
// cl::Buffer db_ac_coef_axiout;    // mm8, output
// cl::Buffer db_strategy_all;      // mm9, output
// cl::Buffer db_raw_quant_field_i; // mm10, output
// cl::Buffer db_hls_order;         // mm11, output
// cl::Buffer db_hls_dc8x8;         // mm12, output
// cl::Buffer db_hls_dc16x16;       // mm13, output
// cl::Buffer db_hls_dc32x32;       // mm14, output
// cl::Buffer db_config;            // mm15, input
// cl::Buffer db_config_fl;         // mm16, input

// // init cl Buffer
// db_hls_opsin_1 = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                             sizeof(float) * ALL_PIXEL, &mext_o[0]);

// db_hls_opsin_2 = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                             sizeof(float) * ALL_PIXEL, &mext_o[1]);

// db_hls_opsin_3 = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                             sizeof(float) * ALL_PIXEL, &mext_o[2]);

// db_hls_quant_field = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                                 sizeof(float) * (BLOCK8_H * BLOCK8_W), &mext_o[3]);

// db_hls_masking_field = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                                   sizeof(float) * (BLOCK8_H * BLOCK8_W), &mext_o[4]);

// db_aq_map_f = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                          sizeof(float) * (BLOCK8_H * BLOCK8_W), &mext_o[5]);

// db_cmap_axi = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                          sizeof(int8_t) * (TILE_W * TILE_H * 2), &mext_o[6]);

// db_ac_coef_axiout = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                                sizeof(int32_t) * ALL_PIXEL, &mext_o[7]);

// db_strategy_all = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                              sizeof(u_char) * (BLOCK8_H * BLOCK8_W), &mext_o[8]);

// db_raw_quant_field_i = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                                   sizeof(float) * (BLOCK8_H * BLOCK8_W), &mext_o[9]);

// db_hls_order = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                           sizeof(float) * MAX_ORDER, &mext_o[10]);

// db_hls_dc8x8 = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                           sizeof(float) * ALL_PIXEL, &mext_o[11]);

// db_hls_dc16x16 = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                             sizeof(float) * ALL_PIXEL, &mext_o[12]);

// db_hls_dc32x32 = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                             sizeof(float) * ALL_PIXEL, &mext_o[13]);

// db_config = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                        sizeof(float) * MAX_NUM_CONFIG, &mext_o[14]);

// db_config_fl = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
//                           sizeof(float) * MAX_NUM_CONFIG, &mext_o[15]);

// //==================================
// // add Buffers to migrate
// std::vector<cl::Memory> ob_in;
// std::vector<cl::Memory> ob_out;

// ob_in.push_back(db_config);
// ob_in.push_back(db_config_fl);
// ob_in.push_back(db_hls_opsin_1);
// ob_in.push_back(db_hls_opsin_2);
// ob_in.push_back(db_hls_opsin_3);
// ob_in.push_back(db_hls_quant_field);
// ob_in.push_back(db_hls_masking_field);
// ob_in.push_back(db_aq_map_f);

// ob_out.push_back(db_cmap_axi);
// ob_out.push_back(db_ac_coef_axiout);
// ob_out.push_back(db_strategy_all);
// ob_out.push_back(db_raw_quant_field_i);
// ob_out.push_back(db_hls_order);
// ob_out.push_back(db_hls_dc8x8);
// ob_out.push_back(db_hls_dc16x16);
// ob_out.push_back(db_hls_dc32x32);

// // set kernel args
// for (int i = 0; i < repInt; i++) {
//     hls_lossy_enc_compute[i].setArg(0, db_config);
//     hls_lossy_enc_compute[i].setArg(1, db_config_fl);
//     hls_lossy_enc_compute[i].setArg(2, db_hls_opsin_1);
//     hls_lossy_enc_compute[i].setArg(3, db_hls_opsin_2);
//     hls_lossy_enc_compute[i].setArg(4, db_hls_opsin_3);
//     hls_lossy_enc_compute[i].setArg(5, db_hls_quant_field);
//     hls_lossy_enc_compute[i].setArg(6, db_hls_masking_field);
//     hls_lossy_enc_compute[i].setArg(7, db_aq_map_f);
//     hls_lossy_enc_compute[i].setArg(8, db_cmap_axi);
//     hls_lossy_enc_compute[i].setArg(9, db_ac_coef_axiout);
//     hls_lossy_enc_compute[i].setArg(10, db_strategy_all);
//     hls_lossy_enc_compute[i].setArg(11, db_raw_quant_field_i);
//     hls_lossy_enc_compute[i].setArg(12, db_hls_order);
//     hls_lossy_enc_compute[i].setArg(13, db_hls_dc8x8);
//     hls_lossy_enc_compute[i].setArg(14, db_hls_dc16x16);
//     hls_lossy_enc_compute[i].setArg(15, db_hls_dc32x32);
// }

// // launch kernel and calculate kernel execution time
// std::cout << "INFO: Kernel Start" << std::endl;
// // declare events
// std::vector<cl::Event> events_write(1);
// std::vector<cl::Event> events_kernel(1);
// std::vector<cl::Event> events_read(1);

// // migrate,
// q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);
// q.enqueueTask(hls_lossy_enc_compute[0], &events_write, &events_kernel[0]);
// q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
// q.finish();

// struct timeval end_time;
// gettimeofday(&end_time, 0);
// std::cout << "INFO: Finish kernel execution" << std::endl;
// std::cout << "INFO: Finish E2E execution" << std::endl;

// // print related times
// unsigned long timeStart, timeEnd, exec_time0;
// std::cout << "-------------------------------------------------------" << std::endl;
// events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
// events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
// exec_time0 = (timeEnd - timeStart) / 1000.0;
// std::cout << "INFO: Data transfer from host to device: " << exec_time0 << " us\n";
// std::cout << "-------------------------------------------------------" << std::endl;
// events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
// events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
// exec_time0 = (timeEnd - timeStart) / 1000.0;
// std::cout << "INFO: Kernel1 Data transfer from device to host: " << exec_time0 << " us\n";
// std::cout << "-------------------------------------------------------" << std::endl;
// exec_time0 = 0;
// for (int i = 0; i < 1; ++i) {
//     events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
//     events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
//     exec_time0 += (timeEnd - timeStart) / 1000.0;

//     std::cout << "INFO: Kernel" << i + 1 << " execution: " << (timeEnd - timeStart) / 1000.0 << " us\n";
//     std::cout << "-------------------------------------------------------" << std::endl;
// }
// std::cout << "INFO: kernel total execution: " << exec_time0 << " us\n";
// std::cout << "-------------------------------------------------------" << std::endl;
// unsigned long exec_timeE2E = diff(&end_time, &start_time);
// std::cout << "INFO: FPGA execution time:" << exec_timeE2E << " us\n";
// std::cout << "-------------------------------------------------------" << std::endl;

// // output
// for (int j = 0; j < TILE_W * TILE_H * 2; j++) {
//     cmap_axi[j] = hb_cmap_axi[j];
// }

// for (int j = 0; j < ALL_PIXEL; j++) {
//     ac_coef_axiout[j] = hb_ac_coef_axiout[j];
// }

// for (int j = 0; j < BLOCK8_W * BLOCK8_H; j++) {
//     strategy_all[j] = hb_strategy_all[j];
// }

// for (int j = 0; j < BLOCK8_H * BLOCK8_W; j++) {
//     raw_quant_field_i[j] = hb_raw_quant_field_i[j];
// }

// for (int j = 0; j < MAX_ORDER; j++) {
//     hls_order[j] = hb_hls_order[j];
// }

// for (int j = 0; j < ALL_PIXEL; j++) {
//     hls_dc8x8[j] = hb_hls_dc8x8[j];
// }

// for (int j = 0; j < ALL_PIXEL; j++) {
//     hls_dc16x16[j] = hb_hls_dc16x16[j];
// }

// for (int j = 0; j < ALL_PIXEL; j++) {
//     hls_dc32x32[j] = hb_hls_dc32x32[j];
// }

// // free mem
// free(hb_hls_opsin_1);
// free(hb_hls_opsin_2);
// free(hb_hls_opsin_3);
// free(hb_hls_quant_field);
// free(hb_hls_masking_field);
// free(hb_aq_map_f);
// free(hb_cmap_axi);
// free(hb_ac_coef_axiout);
// free(hb_strategy_all);
// free(hb_raw_quant_field_i);
// free(hb_hls_order);
// free(hb_hls_dc8x8);
// free(hb_hls_dc16x16);
// free(hb_hls_dc32x32);
// free(hb_config);
// free(hb_config_fl);
#else
    hls_lossy_enc_compute(config, config_fl, hls_opsin_1, hls_opsin_2, hls_opsin_3, hls_quant_field, hls_masking_field,
                          aq_map_f, cmap_axi, ac_coef_axiout, strategy_all, raw_quant_field_i, hls_order, hls_dc8x8,
                          hls_dc16x16, hls_dc32x32);
#endif
}

#endif
