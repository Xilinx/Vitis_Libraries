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
#include "q5.hpp"
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
    int32_t supplier_n = SF1_SUPPLIER;
    int32_t nation_n = SF1_NATION;
    int32_t orders_n = SF1_ORDERS;
    int32_t customer_n = SF1_CUSTOMER;
    int32_t region_n = SF1_REGION;
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        supplier_n = SF30_SUPPLIER;
        nation_n = SF30_NATION;
        orders_n = SF30_ORDERS;
        customer_n = SF30_CUSTOMER;
        region_n = SF30_REGION;
    }

    // ********************************************************** //
    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 6;
    Table tbs[NumTable];
    tbs[0] = Table("region", region_n, 2, in_dir);
    tbs[0].addCol("r_regionkey", 4);
    tbs[0].addCol("r_name", TPCH_READ_REGION_LEN + 1, 0, 0);

    tbs[1] = Table("nation", nation_n, 3, in_dir);
    tbs[1].addCol("n_regionkey", 4);
    tbs[1].addCol("n_nationkey", 4);
    tbs[1].addCol("n_name", TPCH_READ_NATION_LEN + 1);

    tbs[2] = Table("customer", customer_n, 2, in_dir);
    tbs[2].addCol("c_nationkey", 4);
    tbs[2].addCol("c_custkey", 4);

    tbs[3] = Table("order", orders_n, 3, in_dir);
    tbs[3].addCol("o_custkey", 4);
    tbs[3].addCol("o_orderkey", 4);
    tbs[3].addCol("o_orderdate", 4);

    tbs[4] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[4].addCol("l_orderkey", 4);
    tbs[4].addCol("l_suppkey", 4);
    tbs[4].addCol("l_extendedprice", 4);
    tbs[4].addCol("l_discount", 4);

    tbs[5] = Table("supplier", supplier_n, 2, in_dir);
    tbs[5].addCol("s_suppkey", 4);
    tbs[5].addCol("s_nationkey", 4);

    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 180000 * scale, 8, "");
    Table tk0("tk0", 190000 * scale, 8, "");
    Table tk1("tk1", 180000 * scale, 8, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
        std::cout << "DEBUG0" << std::endl;
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

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1, tv_r_2, tv_r_3, tv_r_4, tv_r_5, tv_r_6;
    gettimeofday(&tv_r_s, 0);
    // step1 :region nation-> t1
    q5Join_r_n(tbs[0], tbs[1], th0);
    gettimeofday(&tv_r_0, 0);

    // step2 :t1 customer-> t2
    q5Join_t1_c(th0, tbs[2], tk0);
    gettimeofday(&tv_r_1, 0);

    // step3 : t2 order -> t3
    q5Join_t2_o(tk0, tbs[3], tk1);
    gettimeofday(&tv_r_2, 0);

    // step4 : t3 line -> t4
    q5Join_t3_l(tk1, tbs[4], tk0);
    gettimeofday(&tv_r_3, 0);

    // supplier t4-> t5
    q5Join_s_t4(tbs[5], tk0, tk1);
    gettimeofday(&tv_r_4, 0);

    q5Join_t5_n(tk1, tbs[1], tk0);
    gettimeofday(&tv_r_5, 0);

    q5GroupBy(tk0, tk1);
    gettimeofday(&tv_r_6, 0);

    q5Sort(tk1, tk0);
    gettimeofday(&tv_r_e, 0);
    std::cout << "Join r_n CPU time of Host " << tvdiff(&tv_r_s, &tv_r_0) / 1000 << " ms" << std::endl;
    std::cout << "Join t1_c CPU time of Host " << tvdiff(&tv_r_0, &tv_r_1) / 1000 << " ms" << std::endl;
    std::cout << "Join t2_o CPU time of Host " << tvdiff(&tv_r_1, &tv_r_2) / 1000 << " ms" << std::endl;
    std::cout << "Join t3_l CPU time of Host " << tvdiff(&tv_r_2, &tv_r_3) / 1000 << " ms" << std::endl;
    std::cout << "Join s_t4 CPU time of Host " << tvdiff(&tv_r_3, &tv_r_4) / 1000 << " ms" << std::endl;
    std::cout << "Join t5_n CPU time of Host " << tvdiff(&tv_r_4, &tv_r_5) / 1000 << " ms" << std::endl;
    std::cout << "Groupby CPU time of Host " << tvdiff(&tv_r_5, &tv_r_6) / 1000 << " ms" << std::endl;
    std::cout << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    return 0;
}
