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
#include "q6.hpp"
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
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
    }
    // lineitem_n=200;

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
    const int NumTable = 1;
    const int NumSweep = 1;
    Table tbs[NumTable];

    tbs[0] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[0].addCol("l_extendedprice", 4);
    tbs[0].addCol("l_discount", 4);
    tbs[0].addCol("l_shipdate", 4);
    tbs[0].addCol("l_quantity", 4);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx("tbx", 1024, 1, "");
    Table tk0("tk0", 10, 8, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    tk0.allocateHost();
    tbx.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    cfgCmd cfgcmds;
    cfgcmds.allocateHost();
    get_q6_cfg(cfgcmds.cmd);
    //  get_cfg_dat(cfgcmds[i].cmd,"./host/q10/join/hexBin.dat",i);

    /**
     * 4.allocate device
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateDevBuffer(context, 32);
    }
    tk0.allocateDevBuffer(context, 32);
    tbx.allocateDevBuffer(context, 32);

    std::cout << "DEBUG1" << std::endl;
    cfgcmds.allocateDevBuffer(context, 32);
    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */

    bufferTmp buftmp(context);
    buftmp.initBuffer(q);
    // kernel Engine
    krnlEngine krnlstep;
    krnlstep = krnlEngine(program, q, "gqeJoin");

    krnlstep.setup(tbs[0], tbx, tk0, cfgcmds, buftmp);

    // transfer Engine
    transEngine transin;
    transEngine transout;

    transin.setq(q);
    transout.setq(q);

    transin.add(&(tbs[0]));
    transin.add(&(cfgcmds));
    //  transin.add(&(tbx));

    q.finish();
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

    struct timeval tv_r_s, tv_r_e;
    gettimeofday(&tv_r_s, 0);

    transin.host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    // step1 :region nation-> t1
    krnlstep.run(0, &(eventsh2d_write[0]), &(events[0][0]));
    //  q6(tbs[0],tbx);

    transout.add(&tk0);
    transout.dev2host(0, &(events[0]), &(eventsd2h_read[0][0]));
    q.finish();

    ap_uint<64> res = 0;
    res.range(31, 0) = tk0.getInt32(2, 4);
    res.range(63, 32) = tk0.getInt32(3, 4);
    std::cout << res.to_int64() << std::endl;
    gettimeofday(&tv_r_e, 0);

    cl_ulong kstart;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &kstart);
    print_d_time(eventsh2d_write[0][0], eventsd2h_read[0][0], kstart, "all kernels");
    std::cout << std::dec << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    // q3Print(tk1);
    return 0;
}
