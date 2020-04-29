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
#include "gqe_api.hpp"
#include "q15.hpp"
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

    int board = 0;
    std::string board_str;
    if (parser.getCmdOption("-b", board_str)) {
        try {
            board = std::stoi(board_str);
        } catch (...) {
            board = 0;
        }
    }

    int32_t lineitem_n;
    int32_t supplier_n;
    int32_t customer_n;

    if (mini) {
        lineitem_n = mini;
        supplier_n = SF1_SUPPLIER;
        customer_n = SF1_CUSTOMER;
    } else if (scale == 1) {
        lineitem_n = SF1_LINEITEM;
        supplier_n = SF1_SUPPLIER;
        customer_n = SF1_CUSTOMER;
    } else if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        supplier_n = SF30_SUPPLIER;
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
    cl::Device device = devices[board];
    std::vector<cl::Device> program_device;
    program_device.insert(program_device.begin(), device);

    // Create context and command queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    cl::Program program(context, program_device, xclBins);

    std::cout << "Kernel has been created\n";
    // ********************************************************* //

    /**
     * 1.Table and host cols Created
     */

    // for device table
    const int NumTable = 2;
    Table tbs[NumTable];

    tbs[0] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[0].addCol("l_suppkey", 4);
    tbs[0].addCol("l_extendedprice", 4);
    tbs[0].addCol("l_discount", 4);
    tbs[0].addCol("l_shipdate", 4);

    tbs[1] = Table("supplier", supplier_n, 5, in_dir);
    tbs[1].addCol("s_suppkey", 4);
    tbs[1].addCol("s_name", TPCH_READ_S_NAME_LEN + 1);
    tbs[1].addCol("s_address", TPCH_READ_S_ADDR_MAX + 1);
    tbs[1].addCol("s_phone", TPCH_READ_PHONE_LEN + 1);
    tbs[1].addCol("s_rowid", 4, 1);

    std::cout << "DEBUG0" << std::endl;

    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", customer_n, 16, "");
    Table tk0("tk0", customer_n, 16, "");
    Table tk1("tk1", customer_n, 16, "");
    Table tk2("tk2", customer_n, 16, "");
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
    tk2.allocateHost();

    std::cout << "Table allocation CPU done." << std::endl;

    /**
     * 3. load kernel config from dat and table from disk
     */
    AggrCfgCmd aggrcfgIn;
    AggrCfgCmd aggrcfgOut;

    aggrcfgIn.allocateHost();
    aggrcfgOut.allocateHost();

    get_aggr_cfg(aggrcfgIn.cmd, 0);

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateDevBuffer(context, 32);
    }
    th0.allocateDevBuffer(context, 33);

    aggrcfgOut.allocateDevBuffer(context, 33);
    aggrcfgIn.allocateDevBuffer(context, 32);
    std::cout << "Table allocation device done." << std::endl;

    /**
     * 5.kernels (host and device)
     */
    AggrBufferTmp buftmp(context);

    // kernel Engine
    AggrKrnlEngine krnlstep;
    krnlstep = AggrKrnlEngine(program, q, "gqeAggr");
    krnlstep.setup(tbs[0], th0, aggrcfgIn, aggrcfgOut, buftmp);
    std::cout << "Table allocation device done." << std::endl;

    // transfer Engine
    buftmp.BufferInitial(q);
    transEngine transin;
    transEngine transout;
    std::cout << "Table allocation device done." << std::endl;

    transin.setq(q);
    transout.setq(q);

    q.finish();
    std::cout << "Kernel/Transfer have been setup\n";

    // events
    std::vector<cl::Event> eventsh2d_write;
    std::vector<cl::Event> eventsd2h_read;
    std::vector<cl::Event> events;

    eventsh2d_write.resize(1);
    events.resize(1);
    eventsd2h_read.resize(1);

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0;
#ifdef INI
    th0.initBuffer(q);
#endif
    gettimeofday(&tv_r_s, 0);

    transin.add(&(aggrcfgIn));
    transin.add(&tbs[0]);
    transin.host2dev(0, nullptr, &(eventsh2d_write[0]));

    krnlstep.run(0, &(eventsh2d_write), &(events[0]));

    transout.add(&th0);
    transout.dev2host(0, &(events), &(eventsd2h_read[0]));
    q.finish();
    gettimeofday(&tv_r_0, 0);

    // step4 :find the max key
    int64_t maxv = FindMax(th0);
    q15_filter(th0, tk0, maxv); // host
    q15Join_t1_s(tk0, tbs[1], tk1);
    q15Sort(tk1, tbs[1], th0);

    gettimeofday(&tv_r_e, 0);
    print_h_time(tv_r_s, tv_r_s, tv_r_0, "Kernel");
    print_h_time(tv_r_s, tv_r_0, tv_r_e, "filter..sort..");
    std::cout << std::dec << "All execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
