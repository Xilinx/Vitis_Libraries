#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cstring>
#include <experimental/xrt_bo.h>
#include <experimental/xrt_device.h>
#include <experimental/xrt_kernel.h>

#include "xf_data_mover/bi_dm_helper.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return EXIT_FAILURE;
    }

    char* xclbinFilename = argv[1];
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbinFilename);
    xrt::kernel* dm_krnl = new xrt::kernel(device, uuid, "bi_dm_4d_hsk", true);
    xrt::kernel* s2s_bypass_krnl = new xrt::kernel(device, uuid, "s2s_bypass", true);

    const int cfg_sz = 256 * 1024 * sizeof(uint32_t); // 1MB
    const int data_sz = 128 * 1024 * 1024;            // 128MB
    xrt::bo* cfg_bo = new xrt::bo(device, cfg_sz, dm_krnl->group_id(0));
    xrt::bo* dm_data_in_bo = new xrt::bo(device, data_sz, dm_krnl->group_id(1));
    xrt::bo* dm_data_out_bo = new xrt::bo(device, data_sz, dm_krnl->group_id(2));
    auto cfg_buf = cfg_bo->map<uint32_t*>();
    auto dm_data_in_buf = dm_data_in_bo->map<uint64_t*>();

    const int DATA_WIDTH = 64;
    int cache_len = 65536; // (elem-wise) Maximum num of elements for on-chip cache
    int loop_num = 5;
    int mm2s_bd[] = {0x0, 512};
    int s2mm_bd[] = {0x0C00, 512};
    int cache_loop_bd[] = {0x0C00, 512, 0x0,    512, 0x0C00, 512, 0x0,    512, 0x0C00, 512,
                           0x0,    512, 0x0C00, 512, 0x0,    512, 0x0C00, 512, 0x0,    512};
    int num_sum = 0;

    for (int i = 0; i < loop_num; i++) {
        num_sum += cache_loop_bd[i * 2 + 1];
    }
    auto bidm_helper = new Bi_DM_Helper(cache_len, mm2s_bd, s2mm_bd, loop_num, cache_loop_bd);
    if (!bidm_helper->genCacheLoopCFG<DATA_WIDTH>(cfg_buf, 256 * 1024, false)) {
        std::cout << "ERROR: The input CFG is invalid..." << std::endl;
        return -1;
    }
    auto golden_ptn = bidm_helper->genData(dm_data_in_buf);

    cfg_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, cfg_sz, /*OFFSET=*/0);
    dm_data_in_bo->sync(XCL_BO_SYNC_BO_TO_DEVICE, data_sz, /*OFFSET=*/0);

    uint64_t cfg_bo_addr = cfg_bo->address();
    uint64_t dm_data_in_bo_addr = dm_data_in_bo->address();
    uint64_t dm_data_out_bo_addr = dm_data_out_bo->address();

    dm_krnl->write_register(0x10, cfg_bo_addr);
    dm_krnl->write_register(0x14, (cfg_bo_addr >> 32));
    dm_krnl->write_register(0x1C, dm_data_in_bo_addr);
    dm_krnl->write_register(0x20, (dm_data_in_bo_addr >> 32));
    dm_krnl->write_register(0x28, dm_data_out_bo_addr);
    dm_krnl->write_register(0x2C, (dm_data_out_bo_addr >> 32));

    s2s_bypass_krnl->write_register(0x10, num_sum);

    // start kernel
    dm_krnl->write_register(0x0, 0x1);         // ap_start
    s2s_bypass_krnl->write_register(0x0, 0x1); // ap_start
    while (true) {
        uint32_t status = dm_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    while (true) {
        uint32_t status = s2s_bypass_krnl->read_register(0x0);
        if (status == 6 || status == 4) break;
    }
    dm_krnl->write_register(0x10, 0x1);
    s2s_bypass_krnl->write_register(0x10, 0x1);

    // check result
    int total_nerr = 0;
    bool dump_right = true;
    dm_data_out_bo->sync(XCL_BO_SYNC_BO_FROM_DEVICE, data_sz, 0);
    auto ck_data_buf = dm_data_out_bo->map<uint64_t*>();

    int* current_ptn = golden_ptn + 1;
    int* buf_dim = current_ptn;
    int* offset = current_ptn + 4;
    int* tiling = current_ptn + 8;
    int* dim_idx = current_ptn + 12;
    int* stride = current_ptn + 16;
    int* wrap = current_ptn + 20;
    int nerr = 0;
    for (int w = 0; w < wrap[dim_idx[3]]; w++) {
        for (int z = 0; z < wrap[dim_idx[2]]; z++) {
            for (int y = 0; y < wrap[dim_idx[1]]; y++) {
                for (int x = 0; x < wrap[dim_idx[0]]; x++) {
                    int bias[4];
                    bias[dim_idx[0]] = offset[dim_idx[0]] + stride[dim_idx[0]] * x;
                    bias[dim_idx[1]] = offset[dim_idx[1]] + stride[dim_idx[1]] * y;
                    bias[dim_idx[2]] = offset[dim_idx[2]] + stride[dim_idx[2]] * z;
                    bias[dim_idx[3]] = offset[dim_idx[3]] + stride[dim_idx[3]] * w;
                    for (int l = 0; l < tiling[3]; l++) {
                        for (int k = 0; k < tiling[2]; k++) {
                            for (int j = 0; j < tiling[1]; j++) {
                                for (int i = 0; i < tiling[0]; i++) {
                                    uint64_t golden = (bias[3] + l) * (buf_dim[2] * buf_dim[1] * buf_dim[0]) +
                                                      (bias[2] + k) * (buf_dim[1] * buf_dim[0]) +
                                                      (bias[1] + j) * buf_dim[0] + (bias[0] + i);
                                    uint64_t hw_result = ck_data_buf[golden];
                                    if (golden != hw_result) nerr++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    total_nerr += nerr;
    if (nerr == 0)
        std::cout << "[Test] Check PASSED" << std::endl;
    else
        std::cout << "[Test] nerr: " << std::dec << nerr << std::endl;

    delete dm_krnl;
    delete s2s_bypass_krnl;
    delete cfg_bo;
    delete dm_data_in_bo;
    delete dm_data_out_bo;
    delete bidm_helper;

    return total_nerr;
}
