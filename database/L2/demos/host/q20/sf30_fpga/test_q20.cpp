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
#include "cfg1.hpp"
#include "cfg2.hpp"
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
#include "q20.hpp"
int main(int argc, const char* argv[]) {
    std::cout << "\n------------ TPC-H GQE (1G) -------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string xclbin_path_h; // eg. q5kernel_VCU1525_hw.xclbin
    if (!parser.getCmdOption("-xclbin_h", xclbin_path_h)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }
    std::string xclbin_path_a; // eg. q5kernel_VCU1525_hw.xclbin
    if (!parser.getCmdOption("-xclbin_a", xclbin_path_a)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir) || !is_dir(in_dir)) {
        std::cout << "ERROR: input dir is not specified or not valid.\n";
        return 1;
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

    int psize = 1;
    std::string strPSize;
    if (!parser.getCmdOption("-p", strPSize)) {
        std::cout << "ERROR: partition size is not specified.\n";
    } else {
        psize = std::stoi(strPSize, nullptr);
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
    cl::Device device_h = devices[0];
    cl::Device device_a = devices[1];

    // Create context_h and command queue for selected device
    cl::Context context_h(device_h);
    cl::Context context_a(device_a);
    cl::CommandQueue q_h(context_h, device_h, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    cl::CommandQueue q_a(context_a, device_a, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName_h = device_h.getInfo<CL_DEVICE_NAME>();
    std::string devName_a = device_a.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device_h " << devName_h << "\n";
    std::cout << "Selected Device_a " << devName_a << "\n";

    cl::Program::Binaries xclBins_h = xcl::import_binary_file(xclbin_path_h);
    cl::Program::Binaries xclBins_a = xcl::import_binary_file(xclbin_path_a);
    std::vector<cl::Device> devices_h;
    std::vector<cl::Device> devices_a;
    devices_h.push_back(device_h);
    devices_a.push_back(device_a);
    cl::Program program_h(context_h, devices_h, xclBins_h);
    cl::Program program_a(context_a, devices_a, xclBins_a);

    std::cout << "Kernel has been created\n";

    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 5;
    const int NumSweep = 4;
    Table tbs[NumTable];

    tbs[0] = Table("part", part_n, 2, in_dir);
    tbs[0].addCol("p_partkey", 4);
    tbs[0].addCol("p_name", TPCH_READ_P_NAME_LEN + 1, 0, 0);

    tbs[1] = Table("partsupp", partsupp_n, 3, in_dir);
    tbs[1].addCol("ps_partkey", 4);
    tbs[1].addCol("ps_suppkey", 4);
    tbs[1].addCol("ps_availqty", 4);

    tbs[2] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[2].addCol("l_partkey", 4);
    tbs[2].addCol("l_suppkey", 4);
    tbs[2].addCol("l_shipdate", 4);
    tbs[2].addCol("l_quantity", 4);

    tbs[3] = Table("supplier", supplier_n, 4, in_dir);
    tbs[3].addCol("s_suppkey", 4);
    tbs[3].addCol("s_nationkey", 4);
    tbs[3].addCol("s_address", TPCH_READ_S_ADDR_MAX + 1, 0, 0);
    tbs[3].addCol("s_name", TPCH_READ_S_NAME_LEN + 1, 0, 0);

    tbs[4] = Table("nation", nation_n, 2, in_dir);
    tbs[4].addCol("n_nationkey", 4);
    tbs[4].addCol("n_name", TPCH_READ_NATION_LEN + 1, 0, 0);

    Table pp0("pp0", 1.2 * lineitem_n, 4, "");

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 80000, 1, "");
    Table th1("th1", orders_n / 2, 16, "");
    Table th2("th2", 16, 1, "");
    Table th3("th3", 6400, 2, "");
    Table th4("th4", 100000, 2, "");
    Table tk0("tk0", 300000, 8, "");
    Table tk1("tk1", 300000, 8, "");
    Table tk0_a("tk0", orders_n / 2, 16, "");
    std::cout << "Table Creation done." << std::endl;

    int hjRow = 1 << psize;
    int hpTimes = 1 << (int)ceil(log2((float)(tbs[2].nrow / hjRow)));
    int power_of_hpTimes = log2(hpTimes);
    std::cout << "Number of partition is: " << hpTimes << std::endl;
    const int NumSweep_a = 1 + hpTimes;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    th0.allocateHost();
    th1.allocateHost();
    th2.allocateHost();
    tk0.allocateHost();
    tk1.allocateHost();
    th3.allocateHost();
    th4.allocateHost();
    tk0_a.allocateHost(1.2, hpTimes);
    pp0.allocateHost(1.2, hpTimes);
    // th1.data = tk0_a.data;
    // th1 = tk0_a;

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateHost();
        //    get_cfg_dat(cfgcmds[i].cmd,"hexBin.dat",i);
    };
    get_cfg_dat_1(cfgcmds[0].cmd);
    get_cfg_dat_2(cfgcmds[1].cmd);
    get_cfg_dat_3(cfgcmds[2].cmd);
    get_cfg_dat_4(cfgcmds[3].cmd);

    AggrCfgCmd aggrcfgIn;
    AggrCfgCmd aggrcfgOut[hpTimes];
    cfgCmd hpcfg;

    for (int i = 0; i < hpTimes; i++) {
        aggrcfgOut[i].allocateHost();
    }
    aggrcfgIn.allocateHost();
    hpcfg.allocateHost();

    get_aggr_cfg(aggrcfgIn.cmd, 0);
    get_partition_cfg(hpcfg.cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    tbs[0].allocateDevBuffer(context_h, 32);
    tbs[1].allocateDevBuffer(context_h, 32);
    tbs[2].allocateDevBuffer(context_a, 33);
    tbs[3].allocateDevBuffer(context_h, 32);
    tbs[4].allocateDevBuffer(context_h, 32);
    tk0_a.allocateDevBuffer(context_a, 33);
    pp0.allocateDevBuffer(context_a, 32);
    tk0.allocateDevBuffer(context_h, 32);
    tk1.allocateDevBuffer(context_h, 32);
    th0.allocateDevBuffer(context_h, 32);
    th1.allocateDevBuffer(context_h, 32);
    th2.allocateDevBuffer(context_h, 32);
    th3.allocateDevBuffer(context_h, 32);

    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateDevBuffer(context_h, 32);
    };
    for (int i = 0; i < hpTimes; i++) {
        aggrcfgOut[i].allocateDevBuffer(context_a, 33);
    }
    aggrcfgIn.allocateDevBuffer(context_a, 32);
    hpcfg.allocateDevBuffer(context_a, 32);

    Table tp0[hpTimes];
    for (int i = 0; i < hpTimes; i++) {
        tp0[i] = pp0.createSubTable(i);
    }
    Table tp1[hpTimes];
    for (int i = 0; i < hpTimes; i++) {
        tp1[i] = tk0_a.createSubTable(i);
    }

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    bufferTmp buftmp(context_h);
    buftmp.initBuffer(q_h);
    // kernel Engine
    krnlEngine krnlstep[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        krnlstep[i] = krnlEngine(program_h, q_h, "gqeJoin");
    }
    /////// to add1: add kernel setip
    krnlstep[0].setup(th0, tbs[1], tk0, cfgcmds[0], buftmp);
    krnlstep[1].setup(tk0, th1, tk1, cfgcmds[1], buftmp);
    krnlstep[2].setup(tk1, tbs[3], tk0, cfgcmds[2], buftmp);
    krnlstep[3].setup(tk0, th2, th3, cfgcmds[3], buftmp);

    AggrBufferTmp a_buftmp(context_a);
    a_buftmp.BufferInitial(q_a);

    // kernel Engine
    AggrKrnlEngine a_krnlstep[NumSweep_a];
    a_krnlstep[0] = AggrKrnlEngine(program_a, q_a, "gqePart");
    for (int i = 0; i < hpTimes; i++) {
        a_krnlstep[i + 1] = AggrKrnlEngine(program_a, q_a, "gqeAggr");
    }
    a_krnlstep[0].setup_hp(512, 0, power_of_hpTimes, tbs[2], pp0, hpcfg);
    for (int i = 0; i < hpTimes; i++) {
        a_krnlstep[i + 1].setup(tp0[i], tp1[i], aggrcfgIn, aggrcfgOut[i], a_buftmp);
    }

    // transfer Engine
    transEngine transin[NumSweep];
    transEngine transout[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        transin[i].setq(q_h);
        transout[i].setq(q_h);
    }
    transin[0].add(&(tbs[1]));
    transin[0].add(&(tbs[3]));
    for (int i = 0; i < NumSweep; i++) {
        transin[0].add(&(cfgcmds[i]));
    };
    q_h.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    transEngine a_transin;
    transEngine a_transout[hpTimes];

    a_transin.setq(q_a);
    for (int i = 0; i < hpTimes; i++) {
        a_transout[i].setq(q_a);
    }
    a_transin.add(&(hpcfg));
    a_transin.add(&(aggrcfgIn));
    a_transin.add(&tbs[2]);

    q_a.finish();
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

    std::vector<cl::Event> a_eventsh2d_write;
    std::vector<cl::Event> a_eventsd2h_read[hpTimes];
    std::vector<cl::Event> a_events[NumSweep_a];

    a_eventsh2d_write.resize(1);

    for (int i = 0; i < NumSweep_a; i++) {
        a_events[i].resize(1);
    };

    for (int i = 0; i < hpTimes; i++) {
        a_eventsd2h_read[i].resize(1);
    };
    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1, tv_r_2, tv_r_3, tv_r_4;
#ifdef INI
    tk0.initBuffer(q_h);
    tk1.initBuffer(q_h);
    th3.initBuffer(q_h);
    tk0_a.initBuffer(q_a);
    pp0.initBuffer(q_a);
#endif
    gettimeofday(&tv_r_s, 0);
    a_transin.host2dev(0, nullptr, &(a_eventsh2d_write[0]));

    a_krnlstep[0].run(0, &(a_eventsh2d_write), &(a_events[0][0]));

    for (int i = 0; i < hpTimes; i++) {
        a_krnlstep[i + 1].run(0, &(a_events[0]), &(a_events[i + 1][0]));
    }

    for (int i = 0; i < hpTimes; i++) {
        a_transout[i].add(&tp1[i]);
        a_transout[i].dev2host(0, &(a_events[i + 1]), &(a_eventsd2h_read[i][0]));
    }
    gettimeofday(&tv_r_0, 0);
    Filter_n(tbs[4], th2);
    PartFilter(tbs[0], th0);
    gettimeofday(&tv_r_1, 0);
    transin[0].add(&th2);
    transin[0].add(&th0);
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));
    q_a.finish();
    gettimeofday(&tv_r_2, 0);
    th1.mergeSubTable(tp1, hpTimes);
    GetTable(th1); // extra x10
    gettimeofday(&tv_r_3, 0);

    std::cout << "after filterp " << th0.data[0].range(31, 0).to_int() << std::endl;
    std::cout << "after groupby " << th1.data[0].range(31, 0).to_int() << std::endl;
    std::cout << "after filterf " << th2.data[0].range(31, 0).to_int() << std::endl;

    transin[1].add(&th1);
    transin[1].host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));
    krnlstep[0].run(0, &(eventsh2d_write[1]), &(events[0][0]));

    krnlstep[1].run(0, &(events[0]), &(events[1][0]));

    krnlstep[2].run(0, &(events[1]), &(events[2][0]));

    krnlstep[3].run(0, &(events[2]), &(events[3][0]));
    transout[0].add(&th3);
    transout[0].dev2host(0, &(events[3]), &(eventsd2h_read[0][0]));
    q_h.finish();
    gettimeofday(&tv_r_4, 0);
    std::cout << "th3 row3 " << th3.data[0].range(31, 0).to_int() << std::endl;

    q20_Sort(th3, tbs[3], th4);
    gettimeofday(&tv_r_e, 0);
    cl_ulong kstart;
    a_eventsh2d_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_h_time(tv_r_s, tv_r_s, tv_r_2, "Kernel xclbin_a");
    print_d_time(a_eventsh2d_write[0], a_events[0][0], kstart, "***xclbin_a(to gqepart)");
    print_h_time(tv_r_s, tv_r_0, tv_r_1, "two Filters");
    print_d_time(eventsh2d_write[0][0], eventsh2d_write[0][0], kstart, "xclbin_h trans part");
    print_h_time(tv_r_s, tv_r_2, tv_r_3, "merge and ..");
    print_h_time(tv_r_s, tv_r_3, tv_r_4, "Kernel xclbin_h");
    print_h_time(tv_r_s, tv_r_4, tv_r_e, "Sort");

    std::cout << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
