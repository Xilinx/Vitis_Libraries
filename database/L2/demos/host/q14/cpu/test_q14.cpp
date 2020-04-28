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
#include "q14.hpp"
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
    int32_t part_n = SF1_PART;
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        part_n = SF30_PART;
    }

    // ********************************************************** //

    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 2;
    Table tbs[NumTable];

    tbs[0] = Table("part", part_n, 2, in_dir);
    tbs[0].addCol("p_partkey", 4);
    tbs[0].addCol("p_type", TPCH_READ_P_TYPE_LEN + 1);

    tbs[1] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[1].addCol("l_partkey", 4);
    tbs[1].addCol("l_extendedprice", 4);
    tbs[1].addCol("l_discount", 4);
    tbs[1].addCol("l_shipdate", 4);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 1800000 * scale, 8, "");
    Table tk0("tk0", 180000 * scale, 8, "");
    Table tk1("tk1", 180000 * scale, 8, "");
    Table tk2("tk2", 400000 * scale, 8, "");
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
    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1;
    gettimeofday(&tv_r_s, 0);
    // step1 :filter n-> t1
    PartFilter(tbs[0], th0);
    gettimeofday(&tv_r_0, 0);

    // step2 : orders t1 lineitem-> t2
    int64_t sum1 = q14Join_t1_l(th0, tbs[1], tbx);
    gettimeofday(&tv_r_1, 0);

    // step3 : part lineitem -> t3
    int64_t sum = q14Join_p_l(tbs[0], tbs[1], tbx);

    std::cout << "sum1:" << sum1 << std::endl;
    std::cout << "sum: " << sum << std::endl;
    std::cout << 100 * (double)sum1 / (double)sum << std::endl;

    gettimeofday(&tv_r_e, 0);
    std::cout << "Filter CPU time of Host " << tvdiff(&tv_r_s, &tv_r_0) / 1000 << " ms" << std::endl;
    std::cout << "Join t1_l CPU time of Host " << tvdiff(&tv_r_0, &tv_r_1) / 1000 << " ms" << std::endl;
    std::cout << "Join p_l CPU time of Host " << tvdiff(&tv_r_1, &tv_r_e) / 1000 << " ms" << std::endl;
    std::cout << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
