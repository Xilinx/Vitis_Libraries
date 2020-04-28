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
#include "q22.hpp"
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
    const int NumSweep = 1;
    Table tbs[NumTable];

    tbs[0] = Table("customer", 150000, 3, in_dir);
    tbs[0].addCol("c_custkey", 4);
    tbs[0].addCol("c_phone", TPCH_READ_PHONE_LEN + 1, 0, 0);
    tbs[0].addCol("c_acctbal", 4);

    tbs[1] = Table("orders", 1500000, 1, in_dir);
    tbs[1].addCol("o_custkey", 4);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 160000, 8, "");
    Table tk0("tk0", 70000, 8, "");
    Table tk1("tk1", 7000, 8, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    th0.allocateHost();
    tk0.allocateHost();
    tk1.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    cfgCmd cfgcmds[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateHost();
        //    get_cfg_dat(cfgcmds[i].cmd,"hexBin.dat.q3",i);
    };
    get_cfg_dat_1(cfgcmds[0].cmd);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateDevBuffer(context, 32);
    }
    tk0.allocateDevBuffer(context, 32);
    tk1.allocateDevBuffer(context, 32);
    th0.allocateDevBuffer(context, 32);

    for (int i = 0; i < NumSweep; i++) {
        cfgcmds[i].allocateDevBuffer(context, 32);
    };

    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    bufferTmp buftmp(context);
    buftmp.initBuffer(q);
    // kernel Engine
    krnlEngine krnlstep[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        krnlstep[i] = krnlEngine(program, q, "gqeJoin");
    }

    krnlstep[0].setup(tbs[1], tk0, tk1, cfgcmds[0], buftmp);

    // transfer Engine
    transEngine transin[NumSweep + 1];
    transEngine transout[NumSweep + 1];
    for (int i = 0; i < NumSweep + 1; i++) {
        transin[i].setq(q);
        transout[i].setq(q);
    }
    for (int i = 0; i < NumSweep; i++) {
        transin[0].add(&(cfgcmds[i]));
    };
    q.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write[NumSweep + 1];
    std::vector<cl::Event> eventsd2h_read[NumSweep];
    std::vector<cl::Event> events[NumSweep];
    std::vector<cl::Event> events_grp[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        events[i].resize(1);
    };
    for (int i = 0; i < NumSweep + 1; i++) {
        eventsh2d_write[i].resize(1);
    };
    for (int i = 0; i < NumSweep; i++) {
        eventsd2h_read[i].resize(1);
    };

    struct timeval tv_r_s, tv_r_0, tv_r_1, tv_r_e;
#ifdef INI
    tk1.initBuffer(q);
#endif
    gettimeofday(&tv_r_s, 0);
    transin[0].add(&tbs[1]);
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    gettimeofday(&tv_r_0, 0);
    Substring(tbs[0], th0);
    int32_t avg_acct = Group_AVG(th0, tbx);
    customerFilter_c_acctbal(th0, tk0, avg_acct);
    gettimeofday(&tv_r_1, 0);

    transin[1].add(&tk0);
    transin[1].host2dev(0, &(eventsh2d_write[0]), &(eventsh2d_write[1][0]));

    krnlstep[0].run(0, &(eventsh2d_write[1]), &(events[0][0]));

    transout[0].add(&tk1);
    transout[0].dev2host(0, &(events[0]), &(eventsd2h_read[0][0]));
    q.finish();

    gettimeofday(&tv_r_1, 0);
    q22GroupBy(tk1, tk0);
    q22Sort(tk0, tk1);
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], eventsh2d_write[0][0], kstart, "data trans 0");
    print_h_time(tv_r_s, tv_r_0, tv_r_1, "Substring..");
    print_d_time(eventsh2d_write[1][0], eventsd2h_read[0][0], kstart, "another trans&run&read");
    print_h_time(tv_r_s, tv_r_1, tv_r_e, "Groupby and Sort..");
    std::cout << "All execution time of Host " << std::dec << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
