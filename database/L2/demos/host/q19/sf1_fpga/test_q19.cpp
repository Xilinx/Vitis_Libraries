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
#include "q19.hpp"
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
    const int NumSweep = 3;
    Table tbs[NumTable];

    tbs[0] = Table("part", 200000, 4, in_dir);
    tbs[0].addCol("p_partkey", 4);
    tbs[0].addCol("p_brand", TPCH_READ_P_BRND_LEN + 1, 0, 0);
    tbs[0].addCol("p_container", TPCH_READ_P_CNTR_LEN + 1, 0, 0);
    tbs[0].addCol("p_size", 4);

    tbs[1] = Table("lineitem", 6001215, 6, in_dir);
    tbs[1].addCol("l_partkey", 4);
    tbs[1].addCol("l_quantity", 4);
    tbs[1].addCol("l_extendedprice", 4);
    tbs[1].addCol("l_discount", 4);
    tbs[1].addCol("l_shipmode", TPCH_READ_MAXAGG_LEN + 1, 0, 0);
    tbs[1].addCol("l_shipinstruct", TPCH_READ_MAXAGG_LEN + 1, 0, 0);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 1800000, 8, "");
    Table th1("th1", 1800000, 8, "");
    Table th2("th2", 1800000, 8, "");
    Table tk0("tk0", 300000, 8, "");
    Table tk1("tk1", 64, 8, "");
    Table tk2("tk2", 64, 8, "");
    Table tk3("tk3", 64, 8, "");
    std::cout << "Table Creation done." << std::endl;
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
    tk2.allocateHost();
    tk3.allocateHost();

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
    tk2.allocateDevBuffer(context, 32);
    tk3.allocateDevBuffer(context, 32);
    th0.allocateDevBuffer(context, 32);
    th1.allocateDevBuffer(context, 32);
    th2.allocateDevBuffer(context, 32);

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
    /////// to add1: add kernel setip
    krnlstep[0].setup(th0, tk0, tk1, cfgcmds[0], buftmp);
    krnlstep[1].setup(th1, tk0, tk2, cfgcmds[1], buftmp);
    krnlstep[2].setup(th2, tk0, tk3, cfgcmds[2], buftmp);

    // transfer Engine
    transEngine transin[NumSweep];
    transEngine transout[NumSweep];
    for (int i = 0; i < NumSweep; i++) {
        transin[i].setq(q);
        transout[i].setq(q);
    }

    for (int i = 0; i < NumSweep; i++) {
        transin[0].add(&(cfgcmds[i]));
    };
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
    // step1 :part filter -> t1,t4,t6
    PartFilter1(tbs[0], th0);
    PartFilter2(tbs[0], th1);
    PartFilter3(tbs[0], th2);

    // step2 : lineitem filter -> t2
    LineFilter(tbs[1], tk0);

    transin[0].add(&th0);
    transin[0].add(&th1);
    transin[0].add(&th2);
    transin[0].add(&tk0);
    transin[0].host2dev(0, nullptr, &(eventsh2d_write[0][0]));

    // step3 :t1 t2->t3
    krnlstep[0].run(0, &(eventsh2d_write[0]), &(events[0][0]));
    krnlstep[1].run(0, &(events[0]), &(events[1][0]));
    krnlstep[2].run(0, &(events[1]), &(events[2][0]));

    transout[0].add(&tk1);
    transout[0].add(&tk2);
    transout[0].add(&tk3);
    transout[0].dev2host(0, &(events[2]), &(eventsd2h_read[0][0]));
    q.finish();

    //   q19Join_t1_t2(th0,tk0,tk1);
    //   q19Join_t4_t2(th1,tk0,th0);
    //   q19Join_t6_t2(th2,tk0,th1);

    //   int64_t sum1= q19GroupBy(tk1,tbx);
    //   int64_t sum2= q19GroupBy(th0,tbx);
    //   int64_t sum3= q19GroupBy(th1,tbx);

    ap_int<64> sum1 = 0;
    if (tk1.getNumRow() != 0) {
        sum1.range(31, 0) = tk1.getInt32(2, 0);
        sum1.range(63, 32) = tk1.getInt32(3, 0);
    }
    ap_int<64> sum2 = 0;
    if (tk2.getNumRow() != 0) {
        sum2.range(31, 0) = tk2.getInt32(2, 0);
        sum2.range(63, 32) = tk2.getInt32(3, 0);
    }
    ap_int<64> sum3 = 0;
    if (tk3.getNumRow() != 0) {
        sum3.range(31, 0) = tk3.getInt32(2, 0);
        sum3.range(63, 32) = tk3.getInt32(3, 0);
    }

    std::cout << std::dec << (int64_t)sum1 + (int64_t)sum2 + (int64_t)sum3 << std::endl;
    cl_ulong start1, end1;
    eventsh2d_write[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &start1);
    eventsd2h_read[0][0].getProfilingInfo(CL_PROFILING_COMMAND_END, &end1);
    long kerneltime = (end1 - start1) / 1000000;
    gettimeofday(&tv_r_e, 0);
    std::cout << "Kernel execution time " << kerneltime << " ms" << std::endl;
    std::cout << "CPU execution time of Host " << std::dec << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
