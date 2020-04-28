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
#include "q18.hpp"
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

    // ********************************************************** //
    // Get CL devices.
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device_h = devices[0];
    cl::Device device_a = devices[1];

    // Create context and command queue for selected device
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
    const int NumSweep_h = 3;
    const int NumSweep_a = 1;
    Table tbs[NumTable];

    tbs[0] = Table("lineitem", 6001215, 2, in_dir);
    tbs[0].addCol("l_orderkey", 4);
    tbs[0].addCol("l_quantity", 4);

    tbs[1] = Table("orders", 1500000, 4, in_dir);
    tbs[1].addCol("o_orderkey", 4);
    tbs[1].addCol("o_custkey", 4);
    tbs[1].addCol("o_orderdate", 4);
    tbs[1].addCol("o_totalprice", 4);

    tbs[2] = Table("customer", 150000, 3, in_dir);
    tbs[2].addCol("c_custkey", 4);
    tbs[2].addCol("c_name", TPCH_READ_C_NAME_LEN + 1, 0, 0);
    tbs[2].addCol("c_rowid", 4, 1);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 1600000, 16, "");
    Table th0_a("th0", 1600000, 16, "");
    Table tk0("tk0", 500, 8, "");
    Table tk1("tk1", 500, 8, "");
    Table tbs0 = Table("lineitem", 6001215, 2, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    th0_a.allocateHost();
    th0.allocateHost();
    tk0.allocateHost();
    tk1.allocateHost();
    tbs0.allocateHost();
    tbs0.data = tbs[0].data;
    th0.data = th0_a.data;

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[NumSweep_h];
    for (int i = 0; i < NumSweep_h; i++) {
        cfgcmds[i].allocateHost();
        //  get_cfg_dat(cfgcmds[i].cmd,"hexBin.dat",i);
    };
    get_cfg_dat_1(cfgcmds[0].cmd);
    get_cfg_dat_2(cfgcmds[1].cmd);
    get_cfg_dat_3(cfgcmds[2].cmd);

    AggrCfgCmd a_cfgcmds[NumSweep_a];
    AggrCfgCmd a_cfgcmd_out[NumSweep_a];
    for (int i = 0; i < NumSweep_a; i++) {
        a_cfgcmds[i].allocateHost();
        a_cfgcmd_out[i].allocateHost();
        get_aggr_cfg(a_cfgcmds[i].cmd, i);
    };

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    tbs[0].allocateDevBuffer(context_a, 32);
    tbs0.allocateDevBuffer(context_h, 32);
    tbs[1].allocateDevBuffer(context_h, 32);
    tbs[2].allocateDevBuffer(context_h, 32);
    tk0.allocateDevBuffer(context_h, 32);
    tk1.allocateDevBuffer(context_h, 32);
    th0.allocateDevBuffer(context_h, 32);
    th0_a.allocateDevBuffer(context_a, 33);

    for (int i = 0; i < NumSweep_h; i++) {
        cfgcmds[i].allocateDevBuffer(context_h, 32);
    };

    for (int i = 0; i < NumSweep_a; i++) {
        a_cfgcmds[i].allocateDevBuffer(context_a, 32);
        a_cfgcmd_out[i].allocateDevBuffer(context_a, 33);
    };
    std::cout << "Table allocation device done." << std::endl;
    /**
     * 5.kernels (host and device)
     */

    // transfer Engine
    bufferTmp buftmp(context_h);
    AggrBufferTmp a_buftmp(context_a);
    buftmp.initBuffer(q_h);
    a_buftmp.BufferInitial(q_a);

    krnlEngine krnlstep[NumSweep_h];
    for (int i = 0; i < NumSweep_h; i++) {
        krnlstep[i] = krnlEngine(program_h, q_h, "gqeJoin");
    }
    /////// to add1: add kernel setip
    krnlstep[0].setup(th0, tbs[1], tk0, cfgcmds[0], buftmp);
    krnlstep[1].setup(tk0, tbs[2], tk1, cfgcmds[1], buftmp);
    krnlstep[2].setup(tk1, tbs0, th0, cfgcmds[2], buftmp);

    AggrKrnlEngine a_krnlstep[NumSweep_a];
    for (int i = 0; i < NumSweep_a; i++) {
        a_krnlstep[i] = AggrKrnlEngine(program_a, q_a, "gqeAggr");
    }
    a_krnlstep[0].setup(tbs[0], th0_a, a_cfgcmds[0], a_cfgcmd_out[0], a_buftmp);

    transEngine transin[NumSweep_a + NumSweep_h];
    transEngine transout[NumSweep_a + NumSweep_h];
    for (int i = 0; i < NumSweep_h; i++) {
        transin[i].setq(q_h);
        transout[i].setq(q_h);
    }
    transin[0].add(&(tbs[1]));
    transin[0].add(&(tbs[2]));
    transin[0].add(&(tbs0));
    for (int i = 0; i < NumSweep_h; i++) {
        transin[0].add(&(cfgcmds[i]));
    };
    q_h.finish();

    for (int i = 0; i < NumSweep_a; i++) {
        transin[i + NumSweep_h].setq(q_a);
        transout[i + NumSweep_h].setq(q_a);
    }
    transin[NumSweep_h].add(&(tbs[0]));
    for (int i = 0; i < NumSweep_a; i++) {
        transin[NumSweep_h].add(&(a_cfgcmds[i]));
    };
    q_a.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[NumSweep_h + NumSweep_a];
    std::vector<cl::Event> eventsd2h_read[NumSweep_h + NumSweep_a];
    std::vector<cl::Event> events[NumSweep_h + NumSweep_a];
    for (int i = 0; i < NumSweep_h + NumSweep_a; i++) {
        events[i].resize(1);
    };
    for (int i = 0; i < NumSweep_h + NumSweep_a; i++) {
        eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep_h + NumSweep_a; i++) {
        eventsd2h_read[i].resize(1);
    };

    struct timeval tv_r_s;
    struct timeval tv_r_e;
    struct timeval tv_r_0;
    struct timeval tv_r_1;
#ifdef INI
    tk0.initBuffer(q_h);
    tk1.initBuffer(q_h);
    th0.initBuffer(q_h);
    th0_a.initBuffer(q_a);
#endif
    gettimeofday(&tv_r_s, 0);
    // step1 :H2D
    transin[3].host2dev(0, nullptr, &(eventsh2d_write[3][0]));
    q_a.finish();
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    // step2 :kernel-aggr
    a_krnlstep[0].run(0, &(eventsh2d_write[3]), &(events[3][0]));

    // step3 :D2H
    transout[3].add(&th0_a);
    transout[3].dev2host(0, &(events[3]), &(eventsd2h_read[3][0]));
    q_a.finish();
    gettimeofday(&tv_r_0, 0);

    //  q18GroupBy(tbs[0],th0);
    transin[1].add(&th0);
    transin[1].host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));
    krnlstep[0].run(0, &(eventsh2d_write[1]), &(events[0][0]));
    krnlstep[1].run(0, &(events[0]), &(events[1][0]));
    krnlstep[2].run(0, &(events[1]), &(events[2][0]));
    transout[2].add(&th0);
    transout[2].dev2host(0, &(events[2]), &(eventsd2h_read[2][0]));
    q_h.finish();
    gettimeofday(&tv_r_1, 0);
    std::cout << "After Hash Join" << th0.data[0].range(31, 0).to_int() << std::endl;

    q18GroupBy(th0, tbs[2], tk0);
    q18Sort(tk0, th0);
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    eventsh2d_write[3][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], eventsh2d_write[0][0], kstart, "xclbin_h trans part0");
    print_h_time(tv_r_s, tv_r_s, tv_r_0, "Kernel xclbin_a");
    print_h_time(tv_r_s, tv_r_0, tv_r_1, "Kernel xclbin_h");
    print_h_time(tv_r_s, tv_r_1, tv_r_e, "Group&Sort");
    std::cout << "All execution time of Host " << std::dec << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
