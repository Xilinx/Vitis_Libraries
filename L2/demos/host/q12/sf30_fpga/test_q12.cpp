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
const int PU_NM = 8;
#include "gqe_api.hpp"
#include "q12.hpp"
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
    int board = 0;
    std::string board_s;
    if (parser.getCmdOption("-b", board_s)) {
        try {
            board = std::stoi(board_s);
        } catch (...) {
            board = 0;
        }
    }

    std::string strPSize;
    if (!parser.getCmdOption("-p", strPSize)) {
        std::cout << "ERROR: partition size is not specified.\n";
    }
    int psize = std::stoi(strPSize, nullptr);

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

    int scale = 1;
    std::string scale_str;
    if (parser.getCmdOption("-c", scale_str)) {
        try {
            scale = std::stoi(scale_str);
        } catch (...) {
            scale = 1;
        }
    }
    std::cout << "NOTE:running in sf" << scale << " data\n.";
    int32_t lineitem_n = SF1_LINEITEM;
    int32_t orders_n = SF1_ORDERS;
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        orders_n = SF30_ORDERS;
    }
    // ********************************************************** //

    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[board];

    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    std::vector<cl::Device> devices_;
    devices_.push_back(device);
    cl::Program program(context, devices_, xclBins);

    std::cout << "Kernel has been created\n";
    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 2;
    int NumSweep = 3;
    Table tbs[NumTable];

    tbs[0] = Table("lineitem", lineitem_n, 5, in_dir);
    tbs[0].addCol("l_orderkey", 4);
    tbs[0].addCol("l_shipmode", TPCH_READ_MAXAGG_LEN + 1, 0, 0);
    tbs[0].addCol("l_commitdate", 4);
    tbs[0].addCol("l_receiptdate", 4);
    tbs[0].addCol("l_shipdate", 4);

    tbs[1] = Table("orders", orders_n, 3, in_dir);
    tbs[1].addCol("o_orderkey", 4);
    tbs[1].addCol("o_rowid", 4, 1);
    tbs[1].addCol("o_orderpriority", TPCH_READ_MAXAGG_LEN + 1, 0, 0);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table pp0("pp0", orders_n, 3, "");
    Table pp1("pp1", lineitem_n, 5, "");

    Table tbx(512);
    Table th0("th0", 51424000, 5, "");
    Table tk0("tk0", 1000000, 2, "");
    Table tk1("tk1", 180000 * scale, 8, "");
    Table tk2("tk2", 400000 * scale, 8, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    int hjRow = 1 << psize;
    int hjTimes = 1 << (int)ceil(log2((float)std::max(tbs[0].nrow, tbs[1].nrow) / (float)hjRow));
    int power_of_hjTimes = log2(hjTimes);
    std::cout << "Number of partition is: " << hjTimes << std::endl;

    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    pp0.allocateHost(1.2, hjTimes);
    pp1.allocateHost(1.2, hjTimes);
    tk0.allocateHost(1.2, hjTimes);
    th0.allocateHost();
    tk1.allocateHost();
    tk2.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateHost();
        // get_cfg_dat(cfgcmds[i].cmd, "hexBin.dat", i);
    };
    get_cfg_dat_1p(cfgcmds[0].cmd);
    get_cfg_dat_2j(cfgcmds[1].cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    pp0.allocateDevBuffer(context, 32);
    pp1.allocateDevBuffer(context, 32);

    tbs[1].allocateDevBuffer(context, 33);
    th0.allocateDevBuffer(context, 33);
    tk0.allocateDevBuffer(context, 32);
    // tk1.allocateDevBuffer(context, 32);
    // tk2.allocateDevBuffer(context, 32);

    Table tp0[hjTimes];
    Table tp1[hjTimes];
    Table tp2[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp0[i] = pp0.createSubTable(i);
    }
    for (int i = 0; i < hjTimes; i++) {
        tp1[i] = pp1.createSubTable(i);
    }
    for (int i = 0; i < hjTimes; i++) {
        tp2[i] = tk0.createSubTable(i);
    }

    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateDevBuffer(context, 32);
    };

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    int kernelInd = 0;
    NumSweep = 2 + hjTimes;
    bufferTmp buftmp(context);
    buftmp.initBuffer(q);
    // kernel Engine
    krnlEngine krnlstep[NumSweep];
    krnlstep[kernelInd] = krnlEngine(program, q, "gqePart");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program, q, "gqePart");
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
        kernelInd++;
    }
    /////// to add1: add kernel setip
    kernelInd = 0;
    krnlstep[kernelInd].setup_hp(512, 0, power_of_hjTimes, tbs[1], pp0, cfgcmds[0]); // check config
    kernelInd++;
    krnlstep[kernelInd].setup_hp(512, 1, power_of_hjTimes, th0, pp1, cfgcmds[0]);
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].setup(tp0[i], tp1[i], tp2[i], cfgcmds[1], buftmp);
        kernelInd++;
    }

    // transfer Engine
    transEngine transin[2];
    transEngine transout[2];
    for (int i = 0; i < 2; i++) {
        transin[i].setq(q);
        transout[i].setq(q);
    }
    transin[0].add(&(tbs[1]));
    for (int i = 0; i < 2; i++) {
        transin[0].add(&(cfgcmds[i]));
    };
    q.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[2];
    std::vector<cl::Event> eventsd2h_read[2];
    std::vector<cl::Event> events[2];

    events[0].resize(2);
    events[1].resize(hjTimes);

    for (int i = 0; i < 2; i++) {
        eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < 2; i++) {
        eventsd2h_read[i].resize(1);
    };

    kernelInd = 0;
    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1, tv_r_2;
#ifdef INI
    pp0.initBuffer(q);
    pp1.initBuffer(q);
    tk0.initBuffer(q);
#endif
    gettimeofday(&tv_r_s, 0);
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));
    krnlstep[kernelInd].run(0, &(eventsh2d_write[0]), &(events[0][0]));

    kernelInd++;
    gettimeofday(&tv_r_2, 0);
    LineFilter(tbs[0], th0);
    gettimeofday(&tv_r_0, 0);

    transin[1].add(&th0);
    transin[1].host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));
    krnlstep[kernelInd].run(0, &(eventsh2d_write[1]), &(events[0][1]));
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].run(0, &(events[0]), &(events[1][i]));
        kernelInd++;
    }
    for (int i = 0; i < hjTimes; i++) {
        transout[0].add(&(tp2[i]));
    }
    transout[0].dev2host(0, &(events[1]), &(eventsd2h_read[0][0]));
    q.finish();

    gettimeofday(&tv_r_1, 0);
    q12Groupby(tp2, tbs[0], tbs[1], th0, hjTimes);
    q12Sort(th0, tk0);
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], events[0][0], kstart, "kernel0");
    print_h_time(tv_r_s, tv_r_2, tv_r_0, "LineitemFilter..");
    print_d_time(eventsh2d_write[1][0], eventsd2h_read[0][0], kstart, "kernels...");
    print_h_time(tv_r_s, tv_r_1, tv_r_e, "Groupby and ..");
    std::cout << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
