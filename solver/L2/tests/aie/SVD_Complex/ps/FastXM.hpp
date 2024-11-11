/*
 * MIT License
 *
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Advanced Micro Devices, Inc. shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this Software without prior written authorization from
 * Advanced Micro Devices, Inc.
 */
#ifndef _VCK190_TEST_HARNESS_MGR_HPP_
#define _VCK190_TEST_HARNESS_MGR_HPP_

/*
 * xrt native api based manager
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <cstring>
#include "xrt.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_kernel.h"
#include "xrt/xrt_graph.h"

struct plKernelArg {
    unsigned int arg_idx;
    bool isBO;
    unsigned int size_in_byte;
    char* data;
    unsigned int scalar;
};

class fastXM {
   private:
    xrtDeviceHandle d_hdl;
    xuid_t uuid;
    std::vector<xrtKernelHandle> pl_k_hdl;
    std::vector<std::vector<xrtBufferHandle> > bo_hdl;
    std::vector<std::vector<char*> > bo_src_ptr;
    std::vector<std::vector<unsigned int> > bo_src_size;
    std::vector<std::vector<xrtRunHandle> > pl_k_r_hdl;
    std::vector<xrtGraphHandle> g_hdl;

   public:
    fastXM(unsigned int device_index,
           std::string xclbin_file_path,
           std::vector<std::string> pl_kernel_name,
           std::vector<std::string> graph_name) {
        d_hdl = xrtDeviceOpen(device_index);
        if (d_hdl == nullptr) throw std::runtime_error("ERROR: No valid device handle found! Failed to init device!");
        xrtDeviceLoadXclbinFile(d_hdl, xclbin_file_path.c_str());
        xrtDeviceGetXclbinUUID(d_hdl, uuid);

        for (int i = 0; i < pl_kernel_name.size(); i++) {
            pl_k_hdl.push_back(xrtPLKernelOpen(d_hdl, uuid, pl_kernel_name[i].c_str()));
        }
        bo_hdl.resize(pl_k_hdl.size());
        bo_src_ptr.resize(pl_k_hdl.size());
        bo_src_size.resize(pl_k_hdl.size());
        pl_k_r_hdl.resize(pl_k_hdl.size());
        for (int i = 0; i < graph_name.size(); i++) {
            xrtGraphHandle tmp = xrtGraphOpen(d_hdl, uuid, graph_name[i].c_str());
            if (tmp == NULL) {
                std::cout << "ERROR: graph '" << graph_name[i] << "' is not found! Failed to init device!" << std::endl;
                std::cout << "Please provide check and use correct graph name!" << std::endl;
                exit(1);
            } else {
                g_hdl.push_back(tmp);
            }
        }
        std::cout << "Graph start success!\n";
    }

    void runPL(unsigned int kernel_index, std::vector<plKernelArg> argument) {
        const int& kk = kernel_index;
        auto tmp_pl_k_r_hdl = xrtRunOpen(pl_k_hdl[kk]);

        for (int i = 0; i < argument.size(); i++) {
            if (argument[i].isBO) {
                auto tmp_bo_hdl = xrtBOAlloc(d_hdl, argument[i].size_in_byte, 0,
                                             xrtKernelArgGroupId(pl_k_hdl[kk], argument[i].arg_idx));
                char* tmp_bo_ptr = reinterpret_cast<char*>(xrtBOMap(tmp_bo_hdl));
                memcpy(tmp_bo_ptr, argument[i].data, argument[i].size_in_byte);
                xrtBOSync(tmp_bo_hdl, XCL_BO_SYNC_BO_TO_DEVICE, argument[i].size_in_byte, 0);
                xrtRunSetArg(tmp_pl_k_r_hdl, argument[i].arg_idx, tmp_bo_hdl);
                bo_hdl[kk].push_back(tmp_bo_hdl);
                bo_src_ptr[kk].push_back(argument[i].data);
                bo_src_size[kk].push_back(argument[i].size_in_byte);
            } else {
                xrtRunSetArg(tmp_pl_k_r_hdl, argument[i].arg_idx, argument[i].scalar);
            }
        }
        pl_k_r_hdl[kk].push_back(tmp_pl_k_r_hdl);
        xrtRunStart(tmp_pl_k_r_hdl);
    }

    void runGraph(unsigned int g_idx, unsigned int iters) { xrtGraphRun(g_hdl[g_idx], iters); }

    void waitDone(int graph_timeout_millisec) {
        for (int i = 0; i < pl_k_r_hdl.size(); i++) {
            for (int j = 0; j < pl_k_r_hdl[i].size(); j++) {
                xrtRunWait(pl_k_r_hdl[i][j]);
            }
        }
        for (int i = 0; i < g_hdl.size(); i++) {
            xrtGraphWaitDone(g_hdl[i], graph_timeout_millisec);
        }
    }

    void fetchRes() {
        for (int i = 0; i < bo_hdl.size(); i++) {
            for (int j = 0; j < bo_hdl[i].size(); j++) {
                xrtBOSync(bo_hdl[i][j], XCL_BO_SYNC_BO_FROM_DEVICE, bo_src_size[i][j], 0);
                memcpy(bo_src_ptr[i][j], xrtBOMap(bo_hdl[i][j]), bo_src_size[i][j]);
            }
        }
    }

    void clear() {
        for (int i = 0; i < pl_k_hdl.size(); i++) {
            for (int j = 0; j < pl_k_r_hdl.size(); j++) {
                xrtRunClose(pl_k_r_hdl[i][j]);
            }
            for (int j = 0; j < bo_hdl[i].size(); j++) {
                xrtBOFree(bo_hdl[i][j]);
            }
            bo_src_ptr.clear();
            bo_src_size.clear();
        }
    }

    virtual ~fastXM() {
        std::cout << "~fastXM\n";
        clear();
        for (int i = 0; i < pl_k_hdl.size(); i++) {
            xrtKernelClose(pl_k_hdl[i]);
        }
        for (int i = 0; i < g_hdl.size(); i++) {
            xrtGraphClose(g_hdl[i]);
        }
        xrtDeviceClose(d_hdl);
    }
};

#endif
