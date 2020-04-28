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
//#include "gqe_api.hpp"
#include "q4.hpp"
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

    std::string strPSize;
    if (!parser.getCmdOption("-p", strPSize)) {
        std::cout << "ERROR: partition size is not specified.\n";
    }
    int psize = std::stoi(strPSize, nullptr);

    int board = 0;
    std::string board_s;
    if (parser.getCmdOption("-b", board_s)) {
        try {
            board = std::stoi(board_s);
        } catch (...) {
            board = 0;
        }
    }

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
    //    lineitem_n = 200;
    //    orders_n = 200;

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
    Table tbs[NumTable];

    tbs[0] = Table("lineitem", lineitem_n, 3, in_dir);
    tbs[0].addCol("l_orderkey", 4);
    tbs[0].addCol("l_commitdate", 4);
    tbs[0].addCol("l_receiptdate", 4);

    tbs[1] = Table("orders", orders_n, 4, in_dir);
    tbs[1].addCol("o_orderpriority", TPCH_READ_MAXAGG_LEN + 1, 0, 0);
    tbs[1].addCol("o_orderkey", 4);
    tbs[1].addCol("o_orderdate", 4);
    tbs[1].addCol("o_rowid", 4, 1);

    Table pp0("pp0", lineitem_n, 3, "");
    Table pp1("pp1", orders_n, 3, "");

    Table th0("th0", 1024 * scale, 2, "");
    Table tk0("tk0", 53000 * scale, 1, "");
    Table tk1("tk1", 1024 * scale, 2, "");
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
    tk1.allocateHost();
    th0.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[2];
    cfgcmds[0].allocateHost();
    get_cfg_dat_1(cfgcmds[0].cmd);
    cfgcmds[1].allocateHost();
    get_cfg_dat_1(cfgcmds[1].cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    tbs[0].allocateDevBuffer(context, 33);
    tbs[1].allocateDevBuffer(context, 33);

    //    pp0.allocateDevSBuffer(context);
    //    pp1.allocateDevSBuffer(context);
    //    tk0.allocateDevSBuffer(context);
    pp0.allocateDevBuffer(context, 32);
    pp1.allocateDevBuffer(context, 32);
    tk0.allocateDevBuffer(context, 32);

    Table tp0[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp0[i] = pp0.createSubTable(i);
    }

    Table tp1[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp1[i] = pp1.createSubTable(i);
    }

    Table tp2[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp2[i] = tk0.createSubTable(i);
    }

    cfgcmds[0].allocateDevBuffer(context, 32);
    cfgcmds[1].allocateDevBuffer(context, 32);

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    int kernelInd = 0;
    int NumSweep = 2 + hjTimes;
    krnlEngine krnlstep[NumSweep];
    bufferTmp buftmp(context);
    buftmp.initBuffer(q);
    // kernel Engine
    krnlstep[kernelInd] = krnlEngine(program, q, "gqePart");
    //    krnlstep[kernelInd] = krnlEngine(program, q, "hp_kernel");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program, q, "gqePart");
    //    krnlstep[kernelInd] = krnlEngine(program, q, "hp_kernel");
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
        kernelInd++;
    }

    kernelInd = 0;
    krnlstep[kernelInd].setup_hp(512, 0, power_of_hjTimes, tbs[0], pp0, cfgcmds[0]); // check config
    kernelInd++;
    krnlstep[kernelInd].setup_hp(512, 1, power_of_hjTimes, tbs[1], pp1, cfgcmds[0]);
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].setup(tp0[i], tp1[i], tp2[i], cfgcmds[1], buftmp);
        kernelInd++;
    }

    // transfer Engine
    transEngine transin;
    transEngine transout;

    transin.setq(q);
    transout.setq(q);
    transin.add(&(tbs[0]));
    transin.add(&(tbs[1]));

    transin.add(&(cfgcmds[0]));
    transin.add(&(cfgcmds[1]));
    q.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[NumSweep * 20];
    std::vector<cl::Event> eventsd2h_read[NumSweep * 20];
    std::vector<cl::Event> events[2 * 20];

    for (int i = 0; i < 20; i++) {
        events[0 + i * 2].resize(2);
        events[1 + i * 2].resize(hjTimes);
    }
    struct timeval tv_r_s0, tv_r_s1, tv_r_e;
    transin.host2dev(0, nullptr, nullptr);
    q.finish();
    for (int k = 0; k < 1; k++) {
        for (int i = 0; i < NumSweep * 20; i++) {
            eventsh2d_write[i].resize(1);
        };
        for (int i = 0; i < NumSweep * 20; i++) {
            eventsd2h_read[i].resize(1);
        };

        gettimeofday(&tv_r_s0, 0);

        kernelInd = 0;
        if (k == 0) {
            transin.host2dev(0, nullptr, &(eventsh2d_write[0 + NumSweep * k][0]));
        } else {
            transin.host2dev(0, &(eventsd2h_read[0 + NumSweep * (k - 1)]), &(eventsh2d_write[0 + NumSweep * k][0]));
        }
        krnlstep[kernelInd].run(0, &(eventsh2d_write[0 + NumSweep * k]), &(events[0 + 2 * k][0]));

        kernelInd++;

        krnlstep[kernelInd].run(0, &(eventsh2d_write[0 + NumSweep * k]), &(events[0 + 2 * k][1]));

        kernelInd++;
        for (int i = 0; i < hjTimes; i++) {
            krnlstep[kernelInd].run(0, &(events[0 + 2 * k]), &(events[1 + 2 * k][i]));
            kernelInd++;
        }

        for (int i = 0; i < hjTimes; i++) {
            transout.add(&(tp2[i]));
        }
        transout.dev2host(0, &(events[1 + 2 * k]), &(eventsd2h_read[0 + NumSweep * k][0]));
    }
    q.finish();

    gettimeofday(&tv_r_s1, 0);
    q4GroupBy(tp2, tbs[1], tk1, hjTimes);
    q4Sort(tk1, th0);
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], eventsd2h_read[0][0], kstart, "kernel0");
    print_h_time(tv_r_s0, tv_r_s1, tv_r_e, "Groupby and ..");
    std::cout << std::dec << "All execution time of Host " << tvdiff(&tv_r_s0, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
