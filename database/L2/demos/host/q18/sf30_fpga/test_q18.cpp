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
#include "q18.hpp"
int main(int argc, const char* argv[]) {
    std::cout << "\n------------ TPC-H GQE (1G) -------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir) || !is_dir(in_dir)) {
        std::cout << "ERROR: input dir is not specified or not valid.\n";
        return 1;
    }

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
    int32_t customer_n = SF1_CUSTOMER;
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        orders_n = SF30_ORDERS;
        customer_n = SF30_CUSTOMER;
    }
    // ********************************************************** //

    // Get CL devices.
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
    const int NumTable = 3;
    Table tbs[NumTable];

    tbs[0] = Table("lineitem", lineitem_n, 2, in_dir);
    tbs[0].addCol("l_orderkey", 4);
    tbs[0].addCol("l_quantity", 4);

    tbs[1] = Table("orders", orders_n, 4, in_dir);
    tbs[1].addCol("o_orderkey", 4);
    tbs[1].addCol("o_custkey", 4);
    tbs[1].addCol("o_orderdate", 4);
    tbs[1].addCol("o_totalprice", 4);

    tbs[2] = Table("customer", customer_n, 3, in_dir);
    tbs[2].addCol("c_custkey", 4);
    tbs[2].addCol("c_name", TPCH_READ_C_NAME_LEN + 1, 0, 0);
    tbs[2].addCol("c_rowid", 4, 1);
    Table pp0("pp0", lineitem_n, 2, "");
    Table tbs0 = Table("lineitem", lineitem_n, 2, "");
    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 45000000, 4, "");
    Table th1("th1", 20000, 6, "");
    Table th2("th2", 15000, 6, "");
    Table tk0("tk0", 2400, 4, "");
    Table tk1("tk1", 20000, 5, "");
    Table tk0_a("tk0_a", orders_n, 16, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    int hjRow = 1 << psize;
    int hpTimes = 1 << (int)ceil(log2((float)(tbs[0].nrow / hjRow)));
    int power_of_hpTimes = log2(hpTimes);
    std::cout << "Number of partition is: " << hpTimes << std::endl;
    const int NumSweep_a = 1 + hpTimes;

    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    pp0.allocateHost(1.2, hpTimes);
    tk0_a.allocateHost(1.2, hpTimes);
    th0.allocateHost();
    th1.allocateHost();
    th2.allocateHost();
    tk0.allocateHost();
    tk1.allocateHost();
    tbs0.allocateHost();
    tbs0 = tbs[0];
    // th0.data = tk0_a.data;
    th0 = tk0_a;

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[3];
    for (int i = 0; i < 3; i++) {
        cfgcmds[i].allocateHost();
        // get_cfg_dat(cfgcmds[i].cmd, "hexBin.dat", i);
    };
    get_cfg_dat_1(cfgcmds[0].cmd);
    get_cfg_dat_2(cfgcmds[1].cmd);
    get_cfg_dat_3(cfgcmds[2].cmd);

    AggrCfgCmd aggrcfgIn[hpTimes];
    AggrCfgCmd aggrcfgOut[hpTimes];
    cfgCmd hpcfg;

    for (int i = 0; i < hpTimes; i++) {
        aggrcfgIn[i].allocateHost();
        aggrcfgOut[i].allocateHost();
    }
    hpcfg.allocateHost();

    for (int i = 0; i < hpTimes; i++) {
        get_aggr_cfg(aggrcfgIn[i].cmd, 0);
    }
    get_partition_cfg(hpcfg.cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    tbs0.allocateDevBuffer(context_a, 33);
    tbs[0].allocateDevBuffer(context_h, 32);
    tbs[1].allocateDevBuffer(context_h, 32);
    tbs[2].allocateDevBuffer(context_h, 32);

    tk0.allocateDevBuffer(context_h, 32);
    tk1.allocateDevBuffer(context_h, 32);
    th0.allocateDevBuffer(context_h, 32);
    th2.allocateDevBuffer(context_h, 32);
    tk0_a.allocateDevBuffer(context_a, 33);
    pp0.allocateDevBuffer(context_a, 32);

    for (int i = 0; i < hpTimes; i++) {
        aggrcfgIn[i].allocateDevBuffer(context_a, 32);
        aggrcfgOut[i].allocateDevBuffer(context_a, 33);
    }
    hpcfg.allocateDevBuffer(context_a, 32);

    Table tp0[hpTimes];
    for (int i = 0; i < hpTimes; i++) {
        tp0[i] = pp0.createSubTable(i);
    }
    Table tp1[hpTimes];
    for (int i = 0; i < hpTimes; i++) {
        tp1[i] = tk0_a.createSubTable(i);
    }

    for (int i = 0; i < 3; i++) {
        cfgcmds[i].allocateDevBuffer(context_h, 32);
    };

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    int kernelInd = 0;
    const int NumSweep = 3;
    bufferTmp buftmp(context_h);
    buftmp.initBuffer(q_h);
    // kernel Engine
    krnlEngine krnlstep[NumSweep];
    krnlstep[kernelInd] = krnlEngine(program_h, q_h, "gqeJoin");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program_h, q_h, "gqeJoin");
    kernelInd++;
    krnlstep[kernelInd] = krnlEngine(program_h, q_h, "gqeJoin");
    kernelInd++;

    /////// to add1: add kernel setip
    kernelInd = 0;
    krnlstep[kernelInd].setup(th0, tbs[1], tk0, cfgcmds[0], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup(tk0, tbs[2], tk1, cfgcmds[1], buftmp);
    kernelInd++;
    krnlstep[kernelInd].setup(tk1, tbs[0], th2, cfgcmds[2], buftmp);
    kernelInd++;

    AggrBufferTmp a_buftmp(context_a);
    AggrKrnlEngine a_krnlstep[NumSweep_a];
    a_buftmp.BufferInitial(q_a);
    a_krnlstep[0] = AggrKrnlEngine(program_a, q_a, "gqePart");
    for (int i = 0; i < hpTimes; i++) {
        a_krnlstep[i + 1] = AggrKrnlEngine(program_a, q_a, "gqeAggr");
    }
    a_krnlstep[0].setup_hp(512, 0, power_of_hpTimes, tbs0, pp0, hpcfg);
    for (int i = 0; i < hpTimes; i++) {
        a_krnlstep[i + 1].setup(tp0[i], tp1[i], aggrcfgIn[i], aggrcfgOut[i], a_buftmp);
    }

    transEngine transin[2];
    transEngine transout[2];
    for (int i = 0; i < 2; i++) {
        transin[i].setq(q_h);
        transout[i].setq(q_h);
    }
    for (int i = 0; i < NumTable; i++) {
        transin[0].add(&(tbs[i]));
    };
    for (int i = 0; i < 3; i++) {
        transin[0].add(&(cfgcmds[i]));
    };
    q_h.finish();

    transEngine a_transin;
    transEngine a_transout[hpTimes];
    a_transin.setq(q_a);
    for (int i = 0; i < hpTimes; i++) {
        a_transout[i].setq(q_a);
    }
    a_transin.add(&(tbs0));
    a_transin.add(&(hpcfg));
    for (int i = 0; i < hpTimes; i++) {
        a_transin.add(&(aggrcfgIn[i]));
    }
    q_a.finish();
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
    struct timeval tv_r_s, tv_r_0, tv_r_1, tv_r_e;
#ifdef INI
    tk0.initBuffer(q_h);
    tk1.initBuffer(q_h);
    th2.initBuffer(q_h);
    pp0.initBuffer(q_a);
    tk0_a.initBuffer(q_a);
#endif
    gettimeofday(&tv_r_s, 0);
    a_transin.host2dev(0, nullptr, &(a_eventsh2d_write[0]));
    q_a.finish();
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    // a_krnlstep[0].run(0, &(a_eventsh2d_write), &(a_events[0][0]));
    a_krnlstep[0].run(0, nullptr, &(a_events[0][0]));

    for (int i = 0; i < hpTimes; i++) {
        a_krnlstep[i + 1].run(0, &(a_events[0]), &(a_events[i + 1][0]));
    }

    for (int i = 0; i < hpTimes; i++) {
        a_transout[i].add(&tp1[i]);
        a_transout[i].dev2host(0, &(a_events[i + 1]), &(a_eventsd2h_read[i][0]));
    }
    q_a.finish();
    tk0_a.mergeSubTable(tp1, hpTimes);
    // std::cout<<tk0_a.data[0].range(31,0).to_int()<<std::endl;
    // std::cout<<th0.data[0].range(31,0).to_int()<<std::endl;
    gettimeofday(&tv_r_0, 0);

    kernelInd = 0;
    transin[1].add(&th0);
    transin[1].host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));
    krnlstep[kernelInd].run(0, &(eventsh2d_write[1]), &(events[kernelInd][0]));

    kernelInd++;

    krnlstep[kernelInd].run(0, &(events[kernelInd - 1]), &(events[kernelInd][0]));
    kernelInd++;

    krnlstep[kernelInd].run(0, &(events[kernelInd - 1]), &(events[kernelInd][0]));
    kernelInd++;
    transout[0].add(&(th2));
    transout[0].dev2host(0, &(events[kernelInd - 1]), &(eventsd2h_read[0][0]));
    q_h.finish();
    // std::cout<<th2.data[0].range(31,0).to_int()<<std::endl;

    gettimeofday(&tv_r_1, 0);
    // t4
    q18GroupBy(th2, tbs[2], th1);

    q18Sort(th1, th2);
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], eventsh2d_write[0][0], kstart, "xclbin_h trans part0");
    print_h_time(tv_r_s, tv_r_s, tv_r_0, "Kernel xclbin_a");
    print_h_time(tv_r_s, tv_r_0, tv_r_1, "Kernel xclbin_h");
    print_h_time(tv_r_s, tv_r_1, tv_r_e, "Group&Sort");
    std::cout << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
