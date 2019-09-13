/*
 * Copyright 2019 Xilinx, Inc.
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
#include "table_dt.hpp"
#include "utils.hpp"
#include "cfg.hpp"
#include "tpch_read_2.hpp"

#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <climits>
#include <unordered_map>
#include "gqe_api.hpp"
#include "q1.hpp"
int main(int argc, const char* argv[]) {
    std::cout << "\n------------ TPC-H GQE (1G) -------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string xclbin_path; // eg. q5kernel_VCU1525_hw.xclbin
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir) || !is_dir(in_dir)) {
        std::cout << "ERROR: input dir is not specified or not valid.\n";
        return 1;
    }

    int scale = 1;
    std::string scale_str;
    if (parser.getCmdOption("-c", scale_str)) {
        try {
            scale = std::stoi(scale_str);
        } catch (...) {
            scale = 1;
        }
    }

    int mini = 0;
    std::string mini_str;
    if (parser.getCmdOption("-mini", mini_str)) {
        try {
            mini = std::stoi(mini_str);
        } catch (...) {
            mini = 0;
        }
    }

    int board = 0;
    std::string board_str;
    if (parser.getCmdOption("-b", board_str)) {
        try {
            board = std::stoi(board_str);
        } catch (...) {
            board = 0;
        }
    }

    int32_t lineitem_n;

    if (mini) {
        lineitem_n = mini;
    } else if (scale == 1) {
        lineitem_n = SF1_LINEITEM;
    } else if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
    }
    std::cout << "NOTE:running in sf" << scale << " data\n.";

    int num_rep = 1;
    std::string num_str;
    if (parser.getCmdOption("-rep", num_str)) {
        try {
            num_rep = std::stoi(num_str);
        } catch (...) {
            num_rep = 1;
        }
    }
    if (num_rep > 20) {
        num_rep = 20;
        std::cout << "WARNING: limited repeat to " << num_rep << " times\n.";
    }

    // ********************************************************** //

    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[board];
    std::vector<cl::Device> program_device;
    program_device.insert(program_device.begin(), device);

    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    cl::Program program(context, program_device, xclBins);

    std::cout << "Kernel has been created\n";
    // ********************************************************* //

    /**
     * 1.Table and host cols Created
     */

    // for device table
    const int NumTable = 1;
    const int NumSweep = 1;
    Table tbs[NumTable];
    tbs[0] = Table("lineitem", lineitem_n, 7, in_dir);
    tbs[0].addCol("l_returnflag", 4);
    tbs[0].addCol("l_linestatus", 4);
    tbs[0].addCol("l_quantity", 4);
    tbs[0].addCol("l_extendedprice", 4);
    tbs[0].addCol("l_discount", 4);
    tbs[0].addCol("l_tax", 4);
    tbs[0].addCol("l_shipdate", 4);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table tk0("tk0", 1000, 16, "");
    Table tk1("tk1", 1000, 16, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    tk0.allocateHost();
    tk1.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    AggrCfgCmd cfgcmds[NumSweep];
    AggrCfgCmd cfgcmd_out[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateHost();
        cfgcmd_out[i].allocateHost();

        get_q1_cfg(cfgcmds[i].cmd);
    };

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateDevBuffer(context, 32);
    }
    tk0.allocateDevBuffer(context, 33);
    tk1.allocateDevBuffer(context, 33);

    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateDevBuffer(context, 32);
        cfgcmd_out[i].allocateDevBuffer(context, 33);
    };

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    AggrBufferTmp buftmp(context);

    // kernel Engine
    AggrKrnlEngine krnlstep[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        krnlstep[i] = AggrKrnlEngine(program, q, "gqeAggr");
    }
    krnlstep[0].setup(tbs[0], tk0, cfgcmds[0], cfgcmd_out[0], buftmp);

    // transfer Engine
    buftmp.BufferInitial(q);
    transEngine transin[NumSweep];
    transEngine transout[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        transin[i].setq(q);
        transout[i].setq(q);
    }

    q.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[NumSweep];
    std::vector<cl::Event> eventsd2h_read[NumSweep];
    std::vector<cl::Event> events[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        events[i].resize(1);
    };
    for (int i = 0; i < NumSweep; i++) {
        eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep; i++) {
        eventsd2h_read[i].resize(1);
    };

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_1;
#ifdef INI
    tk0.initBuffer(q);
#endif
    gettimeofday(&tv_r_s, 0);
    // step1 :H2D
    transin[0].add(&tbs[0]);
    transin[0].add(&(cfgcmds[0]));
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    // step2 : kernel-groupby
    krnlstep[0].run(0, &(eventsh2d_write[0]), &(events[0][0]));

    // step3 : D2H
    transout[0].add(&tk0);
    transout[0].dev2host(0, &(events[0]), &(eventsd2h_read[0][0]));
    q.finish();

    // step4 : kernel-join
    gettimeofday(&tv_r_1, 0);
    q1Sort(tk0, tk1);
    gettimeofday(&tv_r_e, 0);
    cl_ulong start1, end1;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &start1);
    eventsd2h_read[0][0].getProfilingInfo(CL_PROFILING_COMMAND_END, &end1);
    long kerneltime1 = (end1 - start1) / 1000000;
    std::cout << std::dec << "Kernel execution time " << kerneltime1 << " ms" << std::endl;
    std::cout << std::dec << "Sort time " << tvdiff(&tv_r_1, &tv_r_e) / 1000 << " ms" << std::endl;
    std::cout << std::dec << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    q1Print(tk1);
    return 0;
}
