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
#include "gqe_api.hpp"
#include "q13.hpp"
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

    int psize_h = 1;
    std::string strPSize_h;
    if (!parser.getCmdOption("-ph", strPSize_h)) {
        std::cout << "ERROR: partition size is not specified.\n";
    } else {
        psize_h = std::stoi(strPSize_h, nullptr);
    }

    int psize_a = 1;
    std::string strPSize_a;
    if (!parser.getCmdOption("-pa", strPSize_a)) {
        std::cout << "ERROR: partition size is not specified.\n";
    } else {
        psize_a = std::stoi(strPSize_a, nullptr);
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
            scale = 0;
        }
    }

    int32_t orders_n;
    int32_t customer_n;

    if (mini) {
        orders_n = mini;
        customer_n = mini;
    } else if (scale == 1) {
        orders_n = SF1_ORDERS;
        customer_n = SF1_CUSTOMER;
    } else if (scale == 30) {
        orders_n = SF30_ORDERS;
        customer_n = SF30_CUSTOMER;
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

    tbs[0] = Table("orders", orders_n, 3, in_dir);
    tbs[0].addCol("o_custkey", 4);
    tbs[0].addCol("o_orderkey", 4);
    tbs[0].addCol("o_comment", TPCH_READ_O_CMNT_MAX + 1);

    tbs[1] = Table("customer", customer_n, 1, in_dir);
    tbs[1].addCol("c_custkey", 4);

    tbs[2] = Table("customer_copy", customer_n, 1, in_dir);
    tbs[2].addCol("c_custkey", 4);

    std::cout << "DEBUG0" << std::endl;

    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table pp0("pp0", orders_n, 2, "");
    Table tk0("tk0", orders_n, 16, "");
    Table tk1("tk1", orders_n, 16, "");
    Table tk3("tk3", 1.2 * orders_n, 16, "");
    Table tk1_h("tk1", 1510000, 2, "");
    Table th0_h("th0", 45000000, 2, "");
    Table tk0_h("tk0", 44400000, 2, "");
    Table pp0_h("pp0", 44400000, 2, "");
    Table pp1_h("pp1", customer_n, 1, "");

    std::cout << "Table Creation done." << std::endl;

    int hjRow_a = 1 << psize_a;
    int hpTimes_a = 1 << (int)ceil(log2((float)(tbs[0].nrow / hjRow_a)));
    int power_of_hpTimes_a = log2(hpTimes_a);
    std::cout << "Aggr Number of partition is: " << hpTimes_a << std::endl;
    const int NumSweep_a = 2 + hpTimes_a;

    int hjTimes_h = psize_h;
    int power_of_hjTimes_h = log2(hjTimes_h);
    const int NumSweep_h = 3 + hjTimes_h;

    std::cout << "Join Number of partition is: " << hjTimes_h << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    pp0.allocateHost(1.2, hpTimes_a);
    tk0.allocateHost(1.2, hpTimes_a);
    tk1.allocateHost();
    tk3.allocateHost();
    // tk1_h.allocateHost(1.2, hjTimes_h);
    tk1_h.allocateHost();
    pp0_h.allocateHost(1.2, hjTimes_h);
    pp1_h.allocateHost(1.2, hjTimes_h);
    tk0_h.allocateHost();
    th0_h.allocateHost();
    tk1 = tk0_h;

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[3];
    for (int i = 0; i < 3; i++) {
        cfgcmds[i].allocateHost();
    };
    get_cfg_dat_1(cfgcmds[0].cmd);
    get_cfg_dat_2(cfgcmds[1].cmd);
    get_cfg_dat_2(cfgcmds[2].cmd);

    AggrCfgCmd aggrcfgIn;
    AggrCfgCmd aggrcfgOut[hpTimes_a];
    cfgCmd hpcfg;

    for (int i = 0; i < hpTimes_a; i++) {
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
    tk0.allocateDevBuffer(context_a, 33);
    tk1.allocateDevBuffer(context_a, 33);
    tk3.allocateDevBuffer(context_a, 32);
    pp0.allocateDevBuffer(context_a, 32);

    for (int i = 0; i < hpTimes_a; i++) {
        aggrcfgOut[i].allocateDevBuffer(context_a, 33);
    }
    aggrcfgIn.allocateDevBuffer(context_a, 32);
    hpcfg.allocateDevBuffer(context_a, 32);

    Table tp0[hpTimes_a];
    for (int i = 0; i < hpTimes_a; i++) {
        tp0[i] = pp0.createSubTable(i);
    }
    Table tp1[hpTimes_a];
    for (int i = 0; i < hpTimes_a; i++) {
        tp1[i] = tk0.createSubTable(i);
    }

    tbs[1].allocateDevBuffer(context_h, 33);
    tbs[2].allocateDevBuffer(context_h, 32);
    tk0_h.allocateDevBuffer(context_h, 33);
    tk1_h.allocateDevBuffer(context_h, 32);
    th0_h.allocateDevBuffer(context_h, 32);

    pp0_h.allocateDevBuffer(context_h, 32);
    pp1_h.allocateDevBuffer(context_h, 32);
    Table tp0_h[hjTimes_h];
    for (int i = 0; i < hjTimes_h; i++) {
        tp0_h[i] = pp0_h.createSubTable(i);
    }

    Table tp1_h[hjTimes_h];
    for (int i = 0; i < hjTimes_h; i++) {
        tp1_h[i] = pp1_h.createSubTable(i);
    }
    Table tp2_h[hjTimes_h];
    for (int i = 0; i < hjTimes_h; i++) {
        tp2_h[i] = tk1_h.createSubTable(i);
    }
    for (int i = 0; i < 3; i++) {
        cfgcmds[i].allocateDevBuffer(context_h, 32);
    };
    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */
    int kernelInd = 0;
    AggrBufferTmp buftmp(context_a);
    buftmp.BufferInitial(q_a);

    // kernel Engine
    AggrKrnlEngine krnlstep[NumSweep_a];
    krnlstep[0] = AggrKrnlEngine(program_a, q_a, "gqePart");
    for (int i = 0; i < hpTimes_a; i++) {
        krnlstep[i + 1] = AggrKrnlEngine(program_a, q_a, "gqeAggr");
    }
    krnlstep[hpTimes_a + 1] = AggrKrnlEngine(program_a, q_a, "gqeAggr");

    krnlstep[0].setup_hp(512, 0, power_of_hpTimes_a, tk1, pp0, hpcfg);
    for (int i = 0; i < hpTimes_a; i++) {
        krnlstep[i + 1].setup(tp0[i], tp1[i], aggrcfgIn, aggrcfgOut[i], buftmp);
    }
    krnlstep[hpTimes_a + 1].setup(tk3, tk0, aggrcfgIn, aggrcfgOut[0], buftmp);

    kernelInd = 0;
    bufferTmp h_buftmp(context_h);
    h_buftmp.initBuffer(q_h);
    // kernel Engine
    krnlEngine h_krnlstep[NumSweep_h];
    h_krnlstep[kernelInd] = krnlEngine(program_h, q_h, "gqeJoin");
    kernelInd++;
    h_krnlstep[kernelInd] = krnlEngine(program_h, q_h, "gqePart");
    kernelInd++;
    h_krnlstep[kernelInd] = krnlEngine(program_h, q_h, "gqePart");
    kernelInd++;
    for (int i = 0; i < hjTimes_h; i++) {
        h_krnlstep[kernelInd] = krnlEngine(program_h, q_h, "gqeJoin");
        kernelInd++;
    }

    kernelInd = 0;
    h_krnlstep[kernelInd].setup(tbs[2], th0_h, tk0_h, cfgcmds[0], h_buftmp);
    kernelInd++;
    h_krnlstep[kernelInd].setup_hp(512, 0, power_of_hjTimes_h, tk0_h, pp0_h, cfgcmds[2]); // check config
    kernelInd++;
    h_krnlstep[kernelInd].setup_hp(512, 1, power_of_hjTimes_h, tbs[1], pp1_h, cfgcmds[2]);
    kernelInd++;
    for (int i = 0; i < hjTimes_h; i++) {
        h_krnlstep[kernelInd].setup(tp0_h[i], tp1_h[i], tk1_h, cfgcmds[1], h_buftmp);
        //        h_krnlstep[kernelInd].setup(tp0_h[i], tp1_h[i], tp2_h[i], cfgcmds[1], h_buftmp);
        kernelInd++;
    }
    // transfer Engine
    transEngine transin[2];
    transEngine transout[hpTimes_a + 1];

    transin[0].setq(q_a);
    transin[1].setq(q_a);
    for (int i = 0; i < hpTimes_a + 1; i++) {
        transout[i].setq(q_a);
    }
    q_a.finish();
    transEngine h_transin[3];
    transEngine h_transout[3];
    for (int i = 0; i < 3; i++) {
        h_transin[i].setq(q_h);
        h_transout[i].setq(q_h);
    }

    h_transin[0].add(&(tbs[2])); // for join
    h_transin[0].add(&(tbs[1])); // for partition

    for (int i = 0; i < 3; i++) {
        h_transin[0].add(&(cfgcmds[i]));
    };
    q_h.finish();
    std::cout << "Kernel/Transfer have been setup\n";
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[2];
    std::vector<cl::Event> eventsd2h_read[NumSweep_a];
    std::vector<cl::Event> events[NumSweep_a];

    eventsh2d_write[0].resize(1);
    eventsh2d_write[1].resize(1);

    for (int i = 0; i < NumSweep_a; i++) {
        events[i].resize(1);
    };

    for (int i = 0; i < NumSweep_a; i++) {
        eventsd2h_read[i].resize(1);
    };

    std::vector<cl::Event> h_eventsh2d_write[NumSweep_h];
    std::vector<cl::Event> h_eventsd2h_read[NumSweep_h];
    std::vector<cl::Event> h_events[NumSweep_h];
    std::vector<cl::Event> h_events_grp[NumSweep_h];
    for (int i = 0; i < NumSweep_h; i++) {
        h_events[i].resize(1);
    };
    h_events[3].resize(hjTimes_h);
    for (int i = 0; i < NumSweep_h; i++) {
        h_eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep_h; i++) {
        h_eventsd2h_read[i].resize(1);
    };

    struct timeval tv_r_s, tv_r_e, tv_r_0, tv_r_1, tv_r_2, tv_r_3, tv_r_4, tv_r_5;
#ifdef INI
    pp0.initBuffer(q_a);
    pp0_h.initBuffer(q_h);
    pp1_h.initBuffer(q_h);
    tk0_h.initBuffer(q_h);
    tk1_h.initBuffer(q_h);
#endif
    kernelInd = 0;
    gettimeofday(&tv_r_s, 0);

    h_transin[0].host2dev(0, nullptr, &(h_eventsh2d_write[0][0]));

    h_krnlstep[2].run(0, &(h_eventsh2d_write[0]), &(h_events[2][0]));

    gettimeofday(&tv_r_0, 0);
    OrderFilter(tbs[0], th0_h); // filter time is long ,so the third kernel data can trans
    gettimeofday(&tv_r_1, 0);

    h_transin[1].add(&th0_h);
    h_transin[1].host2dev(0, &(h_eventsh2d_write[0]), &(h_eventsh2d_write[1][0])); // waitlist is nullptr
    h_krnlstep[0].run(0, &(h_eventsh2d_write[1]), &(h_events[0][0]));

    h_krnlstep[1].run(0, &(h_events[0]), &(h_events[1][0]));

    kernelInd += 3;
    h_events_grp[0].push_back(h_events[1][0]);
    h_events_grp[0].push_back(h_events[2][0]);
    for (int i = 0; i < hjTimes_h; i++) {
        h_krnlstep[kernelInd].run(0, &(h_events_grp[0]), &(h_events[3][i]));
        kernelInd++;
    }

    /* for (int i = 0; i < hjTimes_h; i++) {
           h_transout[0].add(&(tp2_h[i]));
     }*/
    h_transout[0].add(&tk0_h);
    h_transout[0].add(&tk1_h);
    h_transout[0].dev2host(0, &(h_events[3]), &(h_eventsd2h_read[0][0]));
    q_h.finish();
    gettimeofday(&tv_r_2, 0);
    //  tk1_h.mergeSubTable(tp2_h,hjTimes_h);
    //  std::cout<<"after join0 "<<tk0_h.data[0].range(31,0).to_int()<<std::endl;
    //  std::cout<<"after join1 "<<tk1_h.data[0].range(31,0).to_int()<<std::endl;

    // tk0 = tk0_h
    transin[0].add(&(hpcfg));
    transin[0].add(&(aggrcfgIn));
    transin[0].add(&tk1);

    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    // step4 :kernel-partition
    krnlstep[0].run(0, &(eventsh2d_write[0]), &(events[0][0]));

    // step5 :kernel-aggr
    for (int i = 0; i < hpTimes_a; i++) {
        krnlstep[i + 1].run(0, &(events[0]), &(events[i + 1][0]));
    }

    // step6 :D2H
    for (int i = 0; i < hpTimes_a; i++) {
        transout[i].add(&tp1[i]);
        transout[i].dev2host(0, &(events[i + 1]), &(eventsd2h_read[i][0]));
    }
    q_a.finish();
    gettimeofday(&tv_r_3, 0);

    combile_t4_t5(tp1, tk1_h, tk3, hpTimes_a);
    gettimeofday(&tv_r_4, 0);

    transin[1].add(&(hpcfg));
    transin[1].add(&(aggrcfgIn));
    transin[1].add(&tk3);

    transin[1].host2dev(0, nullptr, &(eventsh2d_write[1][0]));

    // step5 :kernel-aggr
    krnlstep[hpTimes_a + 1].run(0, &(eventsh2d_write[1]), &(events[hpTimes_a + 1][0]));
    // step6 :D2H
    transout[hpTimes_a].add(&tk0);
    transout[hpTimes_a].dev2host(0, &(events[hpTimes_a + 1]), &(eventsd2h_read[hpTimes_a][0]));
    q_a.finish();
    gettimeofday(&tv_r_5, 0);

    q13Sort(tk0, tk1);
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    h_eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(h_eventsh2d_write[0][0], h_eventsh2d_write[0][0], kstart, "xclbin_h trans 0");
    print_d_time(h_events[2][0], h_events[2][0], kstart, "xclbin_h kernel2");
    print_h_time(tv_r_s, tv_r_0, tv_r_1, "Orders Filter");
    print_d_time(h_eventsh2d_write[1][0], h_eventsd2h_read[0][0], kstart, "left kernels xclbin_h");
    print_h_time(tv_r_s, tv_r_s, tv_r_2, "All Kernel xclbin_h");
    print_h_time(tv_r_s, tv_r_2, tv_r_3, "Kernel xclbin_a");
    print_h_time(tv_r_s, tv_r_3, tv_r_4, "combine t4_t5");
    print_h_time(tv_r_s, tv_r_4, tv_r_5, "Kernel xclbin_a");
    print_h_time(tv_r_s, tv_r_5, tv_r_e, "Sort");
    std::cout << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;
    return 0;
}
