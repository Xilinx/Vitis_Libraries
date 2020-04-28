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
#include "gqe_api.hpp"
#include "q10.hpp"
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

    int32_t lineitem_n;
    int32_t nation_n;
    int32_t orders_n;
    int32_t customer_n;

    if (mini) {
        lineitem_n = mini;
        nation_n = SF1_NATION;
        orders_n = mini;
        customer_n = SF1_CUSTOMER;
    } else if (scale == 1) {
        lineitem_n = SF1_LINEITEM;
        nation_n = SF1_NATION;
        orders_n = SF1_ORDERS;
        customer_n = SF1_CUSTOMER;
    } else if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        nation_n = SF30_NATION;
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

    const int NumTable = 4;
    const int NumSweep_h = 3;
    Table tbs[NumTable];

    tbs[0] = Table("customer", customer_n, 7, in_dir);
    tbs[0].addCol("c_custkey", 4);
    tbs[0].addCol("c_nationkey", 4);
    tbs[0].addCol("c_rowid", 4, 1);
    tbs[0].addCol("c_name", TPCH_READ_C_NAME_LEN + 1, 0, 0);
    tbs[0].addCol("c_acctbal", 4, 0, 0);
    tbs[0].addCol("c_address", TPCH_READ_C_ADDR_MAX + 1, 0, 0);
    tbs[0].addCol("c_phone", TPCH_READ_PHONE_LEN + 1, 0, 0);

    tbs[1] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[1].addCol("l_returnflag", 4);
    tbs[1].addCol("l_orderkey", 4);
    tbs[1].addCol("l_extendedprice", 4);
    tbs[1].addCol("l_discount", 4);

    tbs[2] = Table("nation", nation_n, 3, in_dir);
    tbs[2].addCol("n_nationkey", 4);
    tbs[2].addCol("n_rowid", 4, 1);
    tbs[2].addCol("n_name", TPCH_READ_NATION_LEN + 1, 0, 0);

    tbs[3] = Table("order", orders_n, 3, in_dir);
    tbs[3].addCol("o_orderdate", 4);
    tbs[3].addCol("o_orderkey", 4);
    tbs[3].addCol("o_custkey", 4);

    std::cout << "DEBUG0" << std::endl;

    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th1("th1", 120000 * scale, 16, "");
    Table tk0("tk0", 120000 * scale, 16, "");
    Table tk1("tk1", 120000 * scale, 16, "");
    Table tk0_h("tk0", 120000 * scale, 8, "");
    Table tk1_h("tk1", 120000 * scale, 8, "");
    Table tk2_h("tk2", 120000 * scale, 5, "");
    std::cout << "Table Creation done." << std::endl;

    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    th1.allocateHost();
    tk0.allocateHost();
    tk1.allocateHost();
    tk0_h.allocateHost();
    tk1_h.allocateHost();
    tk2_h.allocateHost();
    tk0.data = tk2_h.data;
    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    AggrCfgCmd cfgcmds;
    AggrCfgCmd cfgcmd_out;

    cfgcmds.allocateHost();
    cfgcmd_out.allocateHost();
    get_aggr_cfg(cfgcmds.cmd, 0);

    cfgCmd h_cfgcmds[3];
    for (int i = 0; i < 3; i++) {
        h_cfgcmds[i].allocateHost();
    };
    get_cfg_dat_1(h_cfgcmds[0].cmd);
    get_cfg_dat_2(h_cfgcmds[1].cmd);
    get_cfg_dat_3(h_cfgcmds[2].cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    tbs[0].allocateDevBuffer(context_h, 32);
    tbs[1].allocateDevBuffer(context_h, 32);
    tbs[2].allocateDevBuffer(context_h, 32);
    tbs[3].allocateDevBuffer(context_h, 32);

    tk0.allocateDevBuffer(context_a, 32);
    tk1.allocateDevBuffer(context_a, 33);
    th1.allocateDevBuffer(context_a, 33);

    tk0_h.allocateDevBuffer(context_h, 32);
    tk1_h.allocateDevBuffer(context_h, 32);
    tk2_h.allocateDevBuffer(context_h, 32);

    cfgcmds.allocateDevBuffer(context_a, 32);
    cfgcmd_out.allocateDevBuffer(context_a, 33);
    h_cfgcmds[0].allocateDevBuffer(context_h, 32);
    h_cfgcmds[1].allocateDevBuffer(context_h, 32);
    h_cfgcmds[2].allocateDevBuffer(context_h, 32);

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    AggrBufferTmp buftmp(context_a);
    buftmp.BufferInitial(q_a);
    // kernel Engine
    AggrKrnlEngine krnlstep;

    krnlstep = AggrKrnlEngine(program_a, q_a, "gqeAggr");
    krnlstep.setup(tk0, tk1, cfgcmds, cfgcmd_out, buftmp);

    krnlEngine h_krnlstep[3];
    bufferTmp h_buftmp(context_h);
    h_buftmp.initBuffer(q_h);
    // kernel Engine
    h_krnlstep[0] = krnlEngine(program_h, q_h, "gqeJoin");
    h_krnlstep[1] = krnlEngine(program_h, q_h, "gqeJoin");
    h_krnlstep[2] = krnlEngine(program_h, q_h, "gqeJoin");

    h_krnlstep[0].setup(tbs[3], tbs[1], tk0_h, h_cfgcmds[0], h_buftmp);
    h_krnlstep[1].setup(tbs[0], tk0_h, tk1_h, h_cfgcmds[1], h_buftmp);
    h_krnlstep[2].setup(tbs[2], tk1_h, tk2_h, h_cfgcmds[2], h_buftmp);
    // transfer Engine
    transEngine transin;
    transEngine transout;
    transin.setq(q_a);
    transout.setq(q_a);
    q_a.finish();

    transEngine h_transin;
    transEngine h_transout;
    h_transin.setq(q_h);
    h_transout.setq(q_h);
    h_transin.add(&(tbs[0]));
    h_transin.add(&(tbs[1]));
    h_transin.add(&(tbs[2]));
    h_transin.add(&(tbs[3]));
    h_transin.add(&(h_cfgcmds[0]));
    h_transin.add(&(h_cfgcmds[1]));
    h_transin.add(&(h_cfgcmds[2]));
    q_h.finish();
    std::cout << "Kernel/Transfer have been setup." << std::endl;

    // events
    std::vector<cl::Event> eventsh2d_write;
    std::vector<cl::Event> eventsd2h_read;
    std::vector<cl::Event> events;

    events.resize(1);
    eventsh2d_write.resize(1);
    eventsd2h_read.resize(1);

    std::vector<cl::Event> h_eventsh2d_write[NumSweep_h];
    std::vector<cl::Event> h_eventsd2h_read[NumSweep_h];
    std::vector<cl::Event> h_events[NumSweep_h];
    for (int i = 0; i < NumSweep_h; i++) {
        h_events[i].resize(1);
    };
    for (int i = 0; i < NumSweep_h; i++) {
        h_eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep_h; i++) {
        h_eventsd2h_read[i].resize(1);
    };
    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1;
#ifdef INI
    tk0_h.initBuffer(q_h);
    tk1_h.initBuffer(q_h);
    tk2_h.initBuffer(q_h);
    tk1.initBuffer(q_a);
#endif
    gettimeofday(&tv_r_s, 0);

    h_transin.host2dev(0, nullptr, &(h_eventsh2d_write[0][0]));
    h_krnlstep[0].run(0, &(h_eventsh2d_write[0]), &(h_events[0][0]));
    h_krnlstep[1].run(0, &(h_events[0]), &(h_events[1][0]));
    h_krnlstep[2].run(0, &(h_events[1]), &(h_events[2][0]));
    h_transout.add(&tk2_h);
    h_transout.dev2host(0, &(h_events[2]), &(h_eventsd2h_read[0][0]));
    q_h.finish();
    gettimeofday(&tv_r_0, 0);

    //  tk0.data = tk2_h.data;

    transin.add(&tk0);
    transin.add(&(cfgcmds));

    transin.host2dev(0, nullptr, &(eventsh2d_write[0]));

    // step5 : kernel-aggr
    krnlstep.run(0, &(eventsh2d_write), &(events[0]));

    // step6 : D2H
    transout.add(&tk1);
    transout.dev2host(0, &(events), &(eventsd2h_read[0]));
    q_a.finish();
    gettimeofday(&tv_r_1, 0);

    q10Sort(tk1, tk0);
    gathertable(tk0, tbs[0], tbs[2], th1);
    gettimeofday(&tv_r_e, 0);

    print_h_time(tv_r_s, tv_r_s, tv_r_0, "Kernel xclbin_h");
    print_h_time(tv_r_s, tv_r_0, tv_r_1, "Kernel xclbin_a");
    print_h_time(tv_r_s, tv_r_1, tv_r_e, "Sort");
    std::cout << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;
    q10PrintAll(tk0);

    return 0;
}
