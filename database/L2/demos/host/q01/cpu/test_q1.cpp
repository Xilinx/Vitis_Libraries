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
#include "q1.hpp"
int main(int argc, const char* argv[]) {
    std::cout << "\n------------ TPC-H GQE (1G) -------------\n";

    // cmd arg parser.
    ArgParser parser(argc, argv);

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

    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 1;
    Table tbs[NumTable];
    // tbs[0] = Table("lineitem", 6001215, 7, in_dir);
    tbs[0] = Table("lineitem", lineitem_n, 7, in_dir);
    tbs[0].addCol("l_returnflag", 4);
    tbs[0].addCol("l_linestatus", 4);
    tbs[0].addCol("l_quantity", 4);
    tbs[0].addCol("l_extendedprice", 4);
    tbs[0].addCol("l_discount", 4);
    tbs[0].addCol("l_tax", 4);
    tbs[0].addCol("l_shipdate", 4);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("tk0", 6000000 * scale, 20, "");
    Table tk0("tk0", 180000 * scale, 20, "");
    Table tk1("tk1", 180000 * scale, 20, "");
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

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    /**
     * 4.allocate device
     */
    /**
     * 5.kernels (host and device)
     */

    struct timeval tv_r_s, tv_r_e;
    gettimeofday(&tv_r_s, 0);
    // step1 :filter of customer
    q1FilterL(tbs[0], th0);

    // step2 : kernel-join
    q1GroupBy(th0, tk0);

    // step3 : kernel-join
    q1Sort(tk0, tk1);

    // step6 : group

    gettimeofday(&tv_r_e, 0);
    std::cout << std::dec << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    //  q1Print(tk1);
    return 0;
}
