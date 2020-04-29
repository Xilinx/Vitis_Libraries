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
#include <regex>
#include <unordered_map>
const int PU_NM = 8;
#include "gqe_api.hpp"
#include "q21.hpp"
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
    if (scale == 30) {
        lineitem_n = SF30_LINEITEM;
        supplier_n = SF30_SUPPLIER;
        nation_n = SF30_NATION;
        orders_n = SF30_ORDERS;
    }

    // ********************************************************** //

    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 4;
    Table tbs[NumTable];
    tbs[0] = Table("lineitem", lineitem_n, 4, in_dir);
    tbs[0].addCol("l_orderkey", 4);
    tbs[0].addCol("l_suppkey", 4);
    tbs[0].addCol("l_receiptdate", 4);
    tbs[0].addCol("l_commitdate", 4);

    tbs[1] = Table("supplier", supplier_n, 2, in_dir);
    tbs[1].addCol("s_suppkey", 4);
    tbs[1].addCol("s_nationkey", 4);

    tbs[2] = Table("orders", orders_n, 2, in_dir);
    tbs[2].addCol("o_orderkey", 4);
    tbs[2].addCol("o_orderstatus", 4);

    tbs[3] = Table("nation", nation_n, 2, in_dir);
    tbs[3].addCol("n_nationkey", 4);
    tbs[3].addCol("n_name", TPCH_READ_NATION_LEN + 1, 0, 0);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 1500000 * scale, 8, "");
    Table tk0("tk0", 1500000 * scale, 8, "");
    Table tk1("tk1", 400000 * scale, 8, "");
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

    for (int i = 0; i < NumTable; i++) {
        tbs[i].loadHost();
    };

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1, tv_r_2, tv_r_3, tv_r_4, tv_r_5, tv_r_6, tv_r_7;
    gettimeofday(&tv_r_s, 0);

    // step0 :nation supplier-> t0
    NationFilter(tbs[3], tk2);
    gettimeofday(&tv_r_0, 0);

    q21Join_n_s(tk2, tbs[1], th0);
    gettimeofday(&tv_r_1, 0);

    // step1 :t0 lineitem-> t1
    q21Join_t0_l(th0, tbs[0], tk0);
    gettimeofday(&tv_r_2, 0);

    // step2 : orders t1 -> t2
    q21Join_o_t1(tbs[2], tk0, tk1);
    gettimeofday(&tv_r_3, 0);

    // step3: l t2 semijoin -> t3
    q21SemiJoin_l_t2(tbs[0], tk1, tk0);
    gettimeofday(&tv_r_4, 0);

    // step4:l t3 antijoin -> t4
    //  q21AntiJoin_l_t3(tbs[0],tk0,tk1);
    q21SemiJoin_l_t3(tbs[0], tk0, th0);
    gettimeofday(&tv_r_5, 0);

    q21AntiJoin_t4_t3(th0, tk0, tk1);
    gettimeofday(&tv_r_6, 0);

    // step5 : t5
    q21Group_t4(tk1, tk0);
    gettimeofday(&tv_r_7, 0);

    // step6 :sort
    q21Sort(tk0, tk1);

    gettimeofday(&tv_r_e, 0);
    std::cout << "Nation filter CPU time of Host " << tvdiff(&tv_r_s, &tv_r_0) / 1000 << " ms" << std::endl;
    std::cout << "Join n_s CPU time of Host " << tvdiff(&tv_r_0, &tv_r_1) / 1000 << " ms" << std::endl;
    std::cout << "Join t0_l CPU time of Host " << tvdiff(&tv_r_1, &tv_r_2) / 1000 << " ms" << std::endl;
    std::cout << "Join o_t1 CPU time of Host " << tvdiff(&tv_r_2, &tv_r_3) / 1000 << " ms" << std::endl;
    std::cout << "Join l_t2 CPU time of Host " << tvdiff(&tv_r_3, &tv_r_4) / 1000 << " ms" << std::endl;
    std::cout << "Join l_t3 CPU time of Host " << tvdiff(&tv_r_4, &tv_r_5) / 1000 << " ms" << std::endl;
    std::cout << "Join t4_t3 CPU time of Host " << tvdiff(&tv_r_5, &tv_r_6) / 1000 << " ms" << std::endl;
    std::cout << "Groupby CPU time of Host " << tvdiff(&tv_r_6, &tv_r_7) / 1000 << " ms" << std::endl;
    std::cout << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;
    return 0;
}
