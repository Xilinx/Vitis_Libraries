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
#include <set>
const int PU_NM = 8;
#include "gqe_api.hpp"
#include "q16.hpp"
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
    int32_t supplier_n = SF1_SUPPLIER;
    int32_t part_n = SF1_PART;
    int32_t partsupp_n = SF1_PARTSUPP;
    if (scale == 30) {
        supplier_n = SF30_SUPPLIER;
        part_n = SF30_PART;
        partsupp_n = SF30_PARTSUPP;
    }
    // ********************************************************** //
    // ********************************************************* //
    /**
     * 1.Table and host cols Created
     */
    // for device table
    const int NumTable = 3;
    Table tbs[NumTable];
    tbs[0] = Table("part", part_n, 4, in_dir);
    tbs[0].addCol("p_partkey", 4);
    tbs[0].addCol("p_brand", TPCH_READ_P_BRND_LEN + 1);
    tbs[0].addCol("p_type", TPCH_READ_P_TYPE_LEN + 1);
    tbs[0].addCol("p_size", 4);
    //  tbs[0].addCol("p_rowid", 4,1);

    tbs[1] = Table("supplier", supplier_n, 2, in_dir);
    tbs[1].addCol("s_suppkey", 4);
    tbs[1].addCol("s_comment", TPCH_READ_S_CMNT_MAX + 1);

    tbs[2] = Table("partsupp", partsupp_n, 2, in_dir);
    tbs[2].addCol("ps_suppkey", 4);
    tbs[2].addCol("ps_partkey", 4);

    Table constb = Table("const", 8, 1, "");
    constb.addCol("size", 4);

    // tbx is for the empty bufferB in kernel

    Table tbx(512);
    Table th0("th0", 200000 * scale, 8, "");
    Table tk0("tk0", 200000 * scale, 8, "");
    Table tk1("tk1", 200000 * scale, 8, "");
    Table tk2("tk2", 800000 * scale, 8, "");
    std::cout << "Table Creation done." << std::endl;
    /**
     * 2.allocate CPU
     */
    for (int i = 0; i < NumTable; i++) {
        tbs[i].allocateHost();
    }
    constb.allocateHost();
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
    // std::cout<<tbs[2].getNumRow()<<std::endl;

    int consts[8] = {49, 14, 23, 45, 19, 3, 36, 9};
    for (int i = 0; i < 8; i++) {
        // std::cout<<std::dec<<consts[i]<<std::endl;
        constb.setInt32(i, 0, consts[i]);
    }

    struct timeval tv_r_s, tv_r_e;
    struct timeval tv_r_0, tv_r_1, tv_r_2, tv_r_3, tv_r_4, tv_r_5;
    gettimeofday(&tv_r_s, 0);
    // step3 : supplier -> t3
    q16Filter_s(tbs[1], tk1);
    gettimeofday(&tv_r_0, 0);

    // t3 partsupp -> t4
    q16Join_t3_p(tk1, tbs[2], tk2);
    gettimeofday(&tv_r_1, 0);

    // step1 :filter of part -> t1
    q16Filter_p(tbs[0], th0);
    gettimeofday(&tv_r_2, 0);

    // step2 : const t1-> t2
    q16Join_c_t1(constb, th0, tk0);
    gettimeofday(&tv_r_3, 0);

    // t4 t2 -> t5
    q16Join_t4_t2(tk2, tk0, tk1);
    gettimeofday(&tv_r_4, 0);

    // step6 : group sort print #####################to add functions in hpp
    q16GroupBy(tk1, tbs[0], tk0);
    gettimeofday(&tv_r_5, 0);

    q16Sort(tk0, tk1);

    gettimeofday(&tv_r_e, 0);
    std::cout << "Filter s CPU time of Host " << tvdiff(&tv_r_s, &tv_r_0) / 1000 << " ms" << std::endl;
    std::cout << "Join t3_p CPU time of Host " << tvdiff(&tv_r_0, &tv_r_1) / 1000 << " ms" << std::endl;
    std::cout << "Filter p CPU time of Host " << tvdiff(&tv_r_1, &tv_r_2) / 1000 << " ms" << std::endl;
    std::cout << "Join c_t1 CPU time of Host " << tvdiff(&tv_r_2, &tv_r_3) / 1000 << " ms" << std::endl;
    std::cout << "Join t4_t2 CPU time of Host " << tvdiff(&tv_r_3, &tv_r_4) / 1000 << " ms" << std::endl;
    std::cout << "Groupby CPU time of Host " << tvdiff(&tv_r_4, &tv_r_5) / 1000 << " ms" << std::endl;
    std::cout << "CPU execution time of Host " << tvdiff(&tv_r_s, &tv_r_e) / 1000 << " ms" << std::endl;

    // q16Print(tk1);
    return 0;
}
