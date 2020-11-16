/*
 * Copyright 2020 Xilinx, Inc.
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

#ifndef _GQE_JOIN_L3_
#define _GQE_JOIN_L3_

#include <iostream>
#include <thread>
#include <atomic>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <queue>
// L3
#include "xf_database/gqe_ocl.hpp"
#include "xf_database/gqe_join_config.hpp"
#include "xf_database/gqe_join_strategy.hpp"

namespace xf {
namespace database {
namespace gqe {

enum {
    PU_NM = 8,
    VEC_SCAN = 8,
    HT_BUFF_DEPTH = (1 << 25),
    S_BUFF_DEPTH = (1 << 25),
    HBM_BUFF_DEPTH = (1 << 25),
    VEC_LEN = 16,
    KEY_SZ = sizeof(int32_t)
};

class Joiner {
   private:
    // for platform init
    cl_int err;
    cl_context ctx;
    cl_device_id dev_id;
    cl_command_queue cq;
    cl_program prg;
    std::string xclbin_path;

    // solutions:
    // only build + probe
    ErrCode join_sol0(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params);
    // build + pipelined probes

    ErrCode join_sol1(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params);
    // section + partion

    ErrCode join_sol2(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params);
    // join

    ErrCode join_all(Table& tab_a, Table& tab_b, Table& tab_c, JoinConfig& jcmd, std::vector<size_t> params);

   public:
    // constructer
    /**
     * @brief construct of Joiner.
     *
     * @param xclbin xclbin path
     *
     */
    Joiner(std::string xclbin);

    ~Joiner();

    /**
     * @brief join function.
     *
     * Usage:
     *
     * \rst
     * ::
     *
     *   auto sptr = new gqe::JoinStrategyManualSet(solution, sec_o, sec_l, slice_num, log_part, cpu_aggr);
     *   err_code = bigjoin.join(
     *       tab_o, "19940101<=o_orderdate && o_orderdate<19950101",
     *       tab_l, "",
     *       "o_orderkey = l_orderkey",
     *       tab_c1, "c1=l_extendedprice, c2=l_discount, c3=o_orderdate, c4=l_orderkey",
     *       gqe::INNER_JOIN,
     *       sptr);
     *   delete smanual;
     *
     * \endrst
     *
     * Input filter_a/filter_b like "19940101<=o_orderdate && o_orderdate<19950101",
     * o_orderdate and o_orderdate must be exsisted colunm names in table a/b
     * when no filter conditions, input ""
     *
     * Input join conditions like "left_join_key_0=right_join_key_0"
     * when enable dual key join, use comma as seperator,
     * "left_join_key_0=right_join_key_0,left_join_key_1=right_join_key_1"
     *
     * Output strings are like "output_c0 = tab_a_col/tab_b_col",
     * when contains several columns, use comma as seperator
     *
     *
     * @param tab_a left table
     * @param filter_a filter condition of left table
     * @param tab_b right table
     * @param filter_b filter condition of right table
     * @param join_str join condition(s)
     * @param tab_c result table
     * @param output_str output column mapping
     * @param join_type INNER_JOIN(default) | SEMI_JOIN | ANTI_JOIN.
     * @param strategyimp pointer to an object of JoinStrategyBase or its derived type.
     *
     *
     */

    ErrCode join(Table& tab_a,
                 std::string filter_a,
                 Table& tab_b,
                 std::string filter_b,
                 std::string join_str, // comma seprated
                 Table& tab_c,
                 std::string output_str, // comma seprated
                 int join_type = INNER_JOIN,
                 JoinStrategyBase* strategyimp = nullptr);
};
//-----------------------------------------------------------------------------------------------
// put the implementations here for solving compiling deduction

} // gqe
} // database
} // xf
#endif
