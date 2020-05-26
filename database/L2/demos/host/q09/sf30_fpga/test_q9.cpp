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
#include "q9.hpp"
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
    int32_t part_n = SF1_PART;
    int32_t partsupp_n = SF1_PARTSUPP;
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        supplier_n = SF30_SUPPLIER;
        nation_n = SF30_NATION;
        orders_n = SF30_ORDERS;
        part_n = SF30_PART;
        partsupp_n = SF30_PARTSUPP;
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
    const int NumTable = 6;
    // const int NumSweep = 6;
    Table tbs[NumTable];
    tbs[0] = Table("part", part_n, 2, in_dir);
    tbs[0].addCol("p_partkey", 4);
    tbs[0].addCol("p_name", TPCH_READ_P_NAME_LEN + 1, 0, 0);

    tbs[1] = Table("partsupp", partsupp_n, 3, in_dir);
    tbs[1].addCol("ps_partkey", 4);
    tbs[1].addCol("ps_suppkey", 4);
    tbs[1].addCol("ps_supplycost", 4);

    tbs[2] = Table("supplier", supplier_n, 2, in_dir);
    tbs[2].addCol("s_suppkey", 4);
    tbs[2].addCol("s_nationkey", 4);

    tbs[3] = Table("lineitem", lineitem_n, 6, in_dir);
    tbs[3].addCol("l_suppkey", 4);
    tbs[3].addCol("l_partkey", 4);
    tbs[3].addCol("l_orderkey", 4);
    tbs[3].addCol("l_extendedprice", 4);
    tbs[3].addCol("l_discount", 4);
    tbs[3].addCol("l_quantity", 4);

    tbs[4] = Table("orders", orders_n, 2, in_dir);
    tbs[4].addCol("o_orderkey", 4);
    tbs[4].addCol("o_orderdate", 4);

    tbs[5] = Table("nation", nation_n, 3, in_dir);
    tbs[5].addCol("n_nationkey", 4);
    tbs[5].addCol("n_name", TPCH_READ_NATION_LEN + 1, 0, 0);
    tbs[5].addCol("n_rowid", 4, 1);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table pp0("pp0", 12000000, 8, "");
    Table pp1("pp1", orders_n, 2, "");
    Table tbx(512);
    Table th0("th0", 400000, 1, "");
    Table tk0("tk0", 400000 * scale, 8, "");
    Table tk1("tk1", 400000 * scale, 8, "");
    Table tk2("tk2", 400000 * scale, 4, "");
    Table tk3("tk3", 400000 * scale, 8, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    // int hjRow = 1 << psize;
    int hjTimes = psize;
    // int hjTimes = 1 << (int)ceil(log2((float)std::max(tbs[2].nrow, tbs[3].nrow) / (float)hjRow));
    int power_of_hjTimes = log2(hjTimes);

    std::cout << "Number of partition is: " << hjTimes << std::endl;

    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    th0.allocateHost();
    pp0.allocateHost(1.2, hjTimes);
    pp1.allocateHost(1.2, hjTimes);
    tk0.allocateHost();
    tk1.allocateHost(); // hjTimes);
    tk2.allocateHost();
    tk3.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[6];
    for (int i = 0; i < 6; i++) {
        cfgcmds[i].allocateHost();
    };
    get_cfg_dat_1(cfgcmds[0].cmd);
    get_cfg_dat_2(cfgcmds[1].cmd);
    get_cfg_dat_4(cfgcmds[2].cmd);
    get_cfg_dat_3(cfgcmds[3].cmd);
    get_cfg_dat_4(cfgcmds[4].cmd);
    get_cfg_dat_5(cfgcmds[5].cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    tk0.allocateDevBuffer(context, 33);
    tbs[5].allocateDevBuffer(context, 32);
    tbs[4].allocateDevBuffer(context, 33);
    tbs[3].allocateDevBuffer(context, 32);
    tbs[2].allocateDevBuffer(context, 32);
    tbs[1].allocateDevBuffer(context, 32);

    pp0.allocateDevBuffer(context, 32);
    pp1.allocateDevBuffer(context, 32);
    tk1.allocateDevBuffer(context, 32);
    tk2.allocateDevBuffer(context, 32);
    tk3.allocateDevBuffer(context, 32);
    th0.allocateDevBuffer(context, 32);

    Table tp0[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp0[i] = pp0.createSubTable(i);
    }

    Table tp1[hjTimes];
    for (int i = 0; i < hjTimes; i++) {
        tp1[i] = pp1.createSubTable(i);
    }

    /*    Table tp2[hjTimes];
        for (int i = 0; i < hjTimes; i++) {
            tp2[i] = tk1.createSubTable(i);
        }*/

    cfgcmds[0].allocateDevBuffer(context, 32);
    cfgcmds[1].allocateDevBuffer(context, 32);
    cfgcmds[2].allocateDevBuffer(context, 32);
    cfgcmds[3].allocateDevBuffer(context, 32);
    cfgcmds[4].allocateDevBuffer(context, 32);
    cfgcmds[5].allocateDevBuffer(context, 32);

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */
    int kernelInd = 0;
    int NumSweep = 6 + hjTimes;
    krnlEngine krnlstep[NumSweep];
    bufferTmp buftmp(context);
    buftmp.initBuffer(q);
    // kernel Engine
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
    krnlstep[kernelInd] = krnlEngine(program, q, "gqeJoin");

    kernelInd = 0;
    krnlstep[kernelInd].setup(th0, tbs[1], tk1, cfgcmds[0], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup(tbs[2], tk1, tk2, cfgcmds[1], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup(tk2, tbs[3], tk0, cfgcmds[3], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup_hp(512, 0, power_of_hjTimes, tk0, pp0, cfgcmds[2]); // check config
    kernelInd++;
    krnlstep[kernelInd].setup_hp(512, 1, power_of_hjTimes, tbs[4], pp1, cfgcmds[2]);
    kernelInd++;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].setup(tp0[i], tp1[i], tk3, cfgcmds[4], buftmp);
        kernelInd++;
    }
    krnlstep[kernelInd].setup(tbs[5], tk3, tk2, cfgcmds[5], buftmp);

    // transfer Engine
    transEngine transin[NumSweep];
    transEngine transout[NumSweep];

    transin[0].setq(q);
    transin[1].setq(q);
    transin[2].setq(q);
    transout[0].setq(q);
    transin[0].add(&(tbs[5]));
    //    transin[0].add(&(tbs[4]));
    transin[0].add(&(tbs[3]));
    transin[0].add(&(tbs[2]));
    transin[0].add(&(tbs[1]));

    transin[0].add(&(cfgcmds[0]));
    transin[0].add(&(cfgcmds[1]));
    transin[0].add(&(cfgcmds[2]));
    transin[0].add(&(cfgcmds[3]));
    transin[0].add(&(cfgcmds[4]));
    transin[0].add(&(cfgcmds[5]));
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
    for (int i = 0; i < NumSweep; i++) {
        eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep; i++) {
        eventsd2h_read[i].resize(1);
    };

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1;
    struct timeval tv_r_2;
    kernelInd = 0;
#ifdef INI
    tk0.initBuffer(q);
    tk1.initBuffer(q);
    tk2.initBuffer(q);
#endif
    gettimeofday(&tv_r_s, 0);
    // step1 :part filter-> t1
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    gettimeofday(&tv_r_0, 0);
    PartFilter(tbs[0], th0);
    gettimeofday(&tv_r_1, 0);

    transin[1].add(&th0);
    transin[1].host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));
    transin[2].add(&tbs[4]);
    transin[2].host2dev(0, &(eventsh2d_write[1]), &(eventsh2d_write[2][0]));

    // step2 : t1 ps -> t2
    krnlstep[0].run(0, &(eventsh2d_write[1]), &(events[0][0]));

    // step3 : supplier t2 -> t3
    //     q9Join_s_t2(tbs[2], tk0, tk2);
    krnlstep[1].run(0, &(events[0]), &(events[1][0]));

    // step4 : t3 lineitems -> t4
    //     q9Join_t3_l(tk2, tbs[3], tk0);
    krnlstep[2].run(0, &(events[1]), &(events[2][0]));

    // step5 : orders t4 -> t5
    // q9Join_o_t4(tbs[4], tk0, tk1);

    krnlstep[3].run(0, &(events[2]), &(events[4][0]));

    krnlstep[4].run(0, &(eventsh2d_write[2]), &(events[4][1]));

    kernelInd += 5;
    for (int i = 0; i < hjTimes; i++) {
        krnlstep[kernelInd].run(0, &(events[kernelInd - 1]), &(events[kernelInd][0]));
        kernelInd++;
    }

    // step6 : nation t5 -> t6
    //    q9Join_n_t5(tbs[5], tk1, tk2, hjTimes);
    krnlstep[kernelInd].run(0, &(events[kernelInd - 1]), &(events[kernelInd][0]));
    kernelInd++;
    transout[0].add(&tk2);
    transout[0].dev2host(0, &(events[kernelInd - 1]), &(eventsd2h_read[0][0]));
    q.finish();

    gettimeofday(&tv_r_2, 0);

    // step7 :
    q9GroupBy(tk2, tbs[5], tk0);
    // step8 :
    q9Sort(tk0, tk2);
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], eventsh2d_write[0][0], kstart, "data trans 0");
    print_h_time(tv_r_s, tv_r_0, tv_r_1, "PartFilter..");
    print_d_time(eventsh2d_write[1][0], eventsd2h_read[0][0], kstart, "kernel0/1/2/3/4...");
    print_h_time(tv_r_s, tv_r_2, tv_r_e, "Groupby and ..");
    std::cout << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
