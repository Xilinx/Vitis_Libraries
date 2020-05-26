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
#include <regex>
#include <unordered_map>
const int PU_NM = 8;
#include "gqe_api.hpp"
#include "q21.hpp"
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
    int32_t supplier_n = SF1_SUPPLIER;
    int32_t nation_n = SF1_NATION;
    int32_t orders_n = SF1_ORDERS;
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        supplier_n = SF30_SUPPLIER;
        nation_n = SF30_NATION;
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
    const int NumTable = 5;
    Table tbs[NumTable];
    tbs[0] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[0].addCol("l_orderkey", 4);
    tbs[0].addCol("l_suppkey", 4);
    tbs[0].addCol("l_receiptdate", 4);
    tbs[0].addCol("l_commitdate", 4);

    tbs[4] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[4].addCol("l_orderkey", 4);
    tbs[4].addCol("l_suppkey", 4);
    tbs[4].addCol("l_receiptdate", 4);
    tbs[4].addCol("l_commitdate", 4);

    tbs[1] = Table("supplier", supplier_n, 2, in_dir);
    tbs[1].addCol("s_suppkey", 4);
    tbs[1].addCol("s_nationkey", 4);

    tbs[2] = Table("orders", orders_n, 2, in_dir);
    tbs[2].addCol("o_orderkey", 4);
    tbs[2].addCol("o_orderstatus", 4);

    tbs[3] = Table("nation", nation_n, 2, in_dir);
    tbs[3].addCol("n_nationkey", 4);
    tbs[3].addCol("n_name", TPCH_READ_NATION_LEN + 1, 0, 0);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table pp0("pp0", lineitem_n, 4, "");
    Table pp1("pp1", 2200000, 3, "");

    Table tbx(512);
    Table th0("th0", 16, 1, "");
    Table tk0("tk0", 5000000, 3, "");
    Table tk1("tk1", 2500000, 3, "");
    Table tk2("tk2", 12000, 2, "");
    Table tk3("tk3", 2200000, 3, "");
    Table tk4("tk4", 2000000, 3, "");
    Table tk5("tk5", 120000, 2, "");
    std::cout << "Table Creation done." << std::endl;

    /**
     * 2.allocate CPU
     */
    int hjTimes = psize;
    int power_of_hjTimes = log2(hjTimes);

    std::cout << "Number of partition is: " << hjTimes << std::endl;

    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    th0.allocateHost();
    tk0.allocateHost();
    tk1.allocateHost();
    tk2.allocateHost();
    pp0.allocateHost(1.2, hjTimes);
    pp1.allocateHost(1.2, hjTimes);
    tk3.allocateHost(1.2, hjTimes);
    tk4.allocateHost(1.2, hjTimes);
    tk5.allocateHost(1.2, hjTimes);

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[7];
    for (int i = 0; i < 7; i++) {
        cfgcmds[i].allocateHost();
    };
    get_cfg_dat_1(cfgcmds[0].cmd);
    get_cfg_dat_2(cfgcmds[1].cmd);
    get_cfg_dat_3(cfgcmds[2].cmd);
    get_cfg_dat_4p(cfgcmds[6].cmd);
    get_cfg_dat_4j(cfgcmds[3].cmd);
    get_cfg_dat_5(cfgcmds[4].cmd);
    get_cfg_dat_6(cfgcmds[5].cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    tbs[4].allocateDevBuffer(context, 33);
    tk1.allocateDevBuffer(context, 33);
    tbs[0].allocateDevBuffer(context, 32);
    tbs[1].allocateDevBuffer(context, 32);
    tbs[2].allocateDevBuffer(context, 32);

    tk0.allocateDevBuffer(context, 32);
    tk2.allocateDevBuffer(context, 32);
    tk3.allocateDevBuffer(context, 32);
    tk4.allocateDevBuffer(context, 32);
    tk5.allocateDevBuffer(context, 32);
    th0.allocateDevBuffer(context, 32);
    pp0.allocateDevBuffer(context, 32);
    pp1.allocateDevBuffer(context, 32);

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
        tp2[i] = tk3.createSubTable(i);
    }

    Table tp3[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp3[i] = tk4.createSubTable(i);
    }

    Table tp4[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp4[i] = tk5.createSubTable(i);
    }

    for (int i = 0; i < 7; i++) {
        cfgcmds[i].allocateDevBuffer(context, 32);
    };

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    int kernelInd = 0;
    const int NumSweep = 5 + hjTimes * 3;
    bufferTmp buftmp(context);
    buftmp.initBuffer(q);
    // kernel Engine
    krnlEngine krnlstep[NumSweep];
    krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program, q, "gqePart");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program, q, "gqePart");
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
        kernelInd++;
    }
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
        kernelInd++;
    }
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");
        kernelInd++;
    }
    /////// to add1: add kernel setip
    kernelInd = 0;
    krnlstep[kernelInd].setup(th0, tbs[1], tk2, cfgcmds[0], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup(tk2, tbs[0], tk0, cfgcmds[1], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup(tk0, tbs[2], tk1, cfgcmds[2], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup_hp(512, 0, power_of_hjTimes, tbs[4], pp0, cfgcmds[6]); // check config
    kernelInd++;
    krnlstep[kernelInd].setup_hp(512, 1, power_of_hjTimes, tk1, pp1, cfgcmds[6]);
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].setup(tp0[i], tp1[i], tp2[i], cfgcmds[3], buftmp);
        kernelInd++;
    }
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].setup(tp0[i], tp2[i], tp3[i], cfgcmds[4], buftmp);
        kernelInd++;
    }
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].setup(tp3[i], tp2[i], tp4[i], cfgcmds[5], buftmp);
        kernelInd++;
    }

    // transfer Engine
    transEngine transin[4];
    transEngine transout;
    for (int i = 0; i < 4; i++) transin[i].setq(q);
    transout.setq(q);

    for (int i = 0; i < 7; i++) {
        transin[0].add(&(cfgcmds[i]));
    };
    for (int i = 0; i < hjTimes; i++) {
        transout.add(&(tp4[i]));
    }
    q.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[NumSweep];
    std::vector<cl::Event> eventsd2h_read[NumSweep];
    std::vector<cl::Event> events[NumSweep];
    std::vector<cl::Event> events_grp[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        events[i].resize(1);
    };
    events[4].resize(2);
    events[5].resize(hjTimes);
    events[6].resize(hjTimes);
    events[7].resize(hjTimes);
    for (int i = 0; i < NumSweep; i++) {
        eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep; i++) {
        eventsd2h_read[i].resize(1);
    };

    kernelInd = 0;
    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1;
#ifdef INI
    pp0.initBuffer(q);
    pp1.initBuffer(q);
    tk0.initBuffer(q);
    tk1.initBuffer(q);
    tk2.initBuffer(q);
    tk3.initBuffer(q);
    tk4.initBuffer(q);
    tk5.initBuffer(q);
    std::cout << "buffer init before kernel run!" << std::endl;
#endif
    gettimeofday(&tv_r_s, 0);

    NationFilter(tbs[3], th0);
    gettimeofday(&tv_r_0, 0);
    transin[0].add(&(th0));
    transin[0].add(&(tbs[1]));
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));
    krnlstep[0].run(0, &(eventsh2d_write[0]), &(events[0][0]));

    transin[1].add(&(tbs[0]));
    transin[1].host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));
    events_grp[0].push_back(eventsh2d_write[1][0]);
    events_grp[0].push_back(events[0][0]);
    krnlstep[1].run(0, &(events_grp[0]), &(events[1][0]));

    transin[2].add(&(tbs[2]));
    transin[2].host2dev(0, &(eventsh2d_write[1]), &(eventsh2d_write[2][0]));
    transin[3].add(&(tbs[4]));
    transin[3].host2dev(0, &(eventsh2d_write[2]), &(eventsh2d_write[3][0]));

    events_grp[1].push_back(eventsh2d_write[2][0]);
    events_grp[1].push_back(events[1][0]);
    krnlstep[2].run(0, &(events_grp[1]), &(events[2][0]));

    krnlstep[3].run(0, &(eventsh2d_write[3]), &(events[4][0]));
    krnlstep[4].run(0, &(events[2]), &(events[4][1]));

    // step3: l t2 semijoin -> t3
    kernelInd += 5;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].run(0, &(events[4]), &(events[5][i]));
        kernelInd++;
    }

    // step4:l t3 antijoin -> t4
    // q21SemiJoin_l_t3(tbs[0], tp2, hjTimes, tk0);
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].run(0, &(events[5]), &(events[6][i]));
        kernelInd++;
    }

    // q21AntiJoin_t4_t3(tk0, tp2, hjTimes, tk1);
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].run(0, &(events[6]), &(events[7][i]));
        kernelInd++;
    }

    // Transfer out
    transout.dev2host(0, &(events[7]), &(eventsd2h_read[0][0]));
    q.finish();

    // step5 : t5
    gettimeofday(&tv_r_1, 0);
    q21Group_t4(tp4, hjTimes, tk0);
    // q21Group_t4(tk1, tk0);
    // step6 :sort
    q21Sort(tk0, tk1);
    gettimeofday(&tv_r_e, 0);

    print_h_time(tv_r_s, tv_r_s, tv_r_0, "NationFilter..");
    int64_t offset = tvdiff(&tv_r_s, &tv_r_0) / 1000;
    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], events[0][0], kstart, "kernel0", offset);
    print_d_time(eventsh2d_write[1][0], events[1][0], kstart, "kernel1", offset);
    print_d_time(eventsh2d_write[2][0], events[2][0], kstart, "kernel2", offset);
    print_d_time(eventsh2d_write[3][0], events[4][0], kstart, "kernel3", offset);
    print_d_time(events[4][1], eventsd2h_read[0][0], kstart, "kernel4...", offset);
    print_d_time(eventsh2d_write[0][0], eventsd2h_read[0][0], kstart, "all kernels", offset);
    print_h_time(tv_r_s, tv_r_1, tv_r_e, "Group&Sort..");
    std::cout << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;
    return 0;
}
