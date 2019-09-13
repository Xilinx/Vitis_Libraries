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
#include "q13.hpp"
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
    int32_t orders_n = SF1_ORDERS;
    int32_t customer_n = SF1_CUSTOMER;
    if (scale == 30) {
        orders_n = SF30_ORDERS;
        customer_n = SF30_CUSTOMER;
    }
    // ********************************************************** //
    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 2;
    Table tbs[NumTable];

    tbs[0] = Table("orders", orders_n, 3, in_dir);
    tbs[0].addCol("o_custkey", 4);
    tbs[0].addCol("o_orderkey", 4);
    tbs[0].addCol("o_comment", TPCH_READ_O_CMNT_MAX + 1);

    tbs[1] = Table("customer", customer_n, 1, in_dir);
    tbs[1].addCol("c_custkey", 4);

    std::cout << "DEBUG0" << std::endl;
    // tbx is for the empty bufferB in kernel
    Table tbx(512);
    Table th0("th0", 1800000 * scale, 8, "");
    Table tk0("tk0", 1800000 * scale, 8, "");
    Table tk1("tk1", 1800000 * scale, 8, "");
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

    struct timeval tv_r_s, tv_r_e, tv_s_0, tv_s_1, tv_s_2, tv_s_3, tv_s_4, tv_s_5, tv_s_6;
    gettimeofday(&tv_r_s, 0);
    // step1 :filter of order -> t1
    OrderFilter(tbs[0], th0);

    gettimeofday(&tv_s_0, 0);
    // step2 :customer, t1-> t2
    q13Join_c_t1(tbs[1], th0, tk0);

    gettimeofday(&tv_s_1, 0);
    // step3 :t2 customer -> t3
    q13AntiJoin_t2_c(tk0, tbs[1], tk1);

    gettimeofday(&tv_s_2, 0);
    // step3 :
    q13GroupBy_t2(tk0, th0);

    gettimeofday(&tv_s_3, 0);
    // step3 :
    q13GroupBy_t3(tk1, tk0);

    gettimeofday(&tv_s_4, 0);
    // t5 t4->t6 (o_custkey,c_count)
    combile_t4_t5(th0, tk0, tk1);
    // group t6
    gettimeofday(&tv_s_5, 0);

    q13GroupBy(tk1, tk0);

    gettimeofday(&tv_s_6, 0);

    q13Sort(tk0, tk1);
    gettimeofday(&tv_r_e, 0);
    std::cout << "Filter time of Host " << tvdiff(&tv_r_s, &tv_s_0) / 1000 << " ms" << std::endl;
    std::cout << "Join time of Host " << tvdiff(&tv_s_0, &tv_s_1) / 1000 << " ms" << std::endl;
    std::cout << "Anti Join time of Host " << tvdiff(&tv_s_1, &tv_s_2) / 1000 << " ms" << std::endl;
    std::cout << "Groupby_t2 time of Host " << tvdiff(&tv_s_2, &tv_s_3) / 1000 << " ms" << std::endl;
    std::cout << "Groupby_t3 time of Host " << tvdiff(&tv_s_3, &tv_s_4) / 1000 << " ms" << std::endl;
    std::cout << "combine_t4_t5 time of Host " << tvdiff(&tv_s_4, &tv_s_5) / 1000 << " ms" << std::endl;
    std::cout << "q13Groupby time of Host " << tvdiff(&tv_s_5, &tv_s_6) / 1000 << " ms" << std::endl;
    std::cout << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;
    return 0;
}
