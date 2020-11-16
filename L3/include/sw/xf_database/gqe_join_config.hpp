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

#ifndef _GQE_JOIN_CONFIG_L3_
#define _GQE_JOIN_CONFIG_L3_
// commmon
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <iomanip>
// HLS
#include <ap_int.h>
// L2
#include "xf_database/gqe_utils.hpp"
// L3
#include "xf_database/gqe_table.hpp"

namespace xf {
namespace database {
namespace gqe {
enum { INNER_JOIN = 0, SEMI_JOIN = 1, ANTI_JOIN = 2 };

class JoinConfig {
   private:
    // for cfg
    gqe::utils::MM mm;
    std::vector<std::vector<std::string> > join_keys;
    // std::vector<std::vector<std::string> > filter_keys;
    std::vector<std::string> write_cols;
    std::vector<std::string> write_cols_out;
    std::vector<std::vector<std::string> > eval_cols;
    // sol2 part scan, sol2 join scan and sw_shuffle from origin input
    std::vector<std::vector<int8_t> > scan_sw_shuf1;
    // sol2 join scan and sw_shuffle from part out
    std::vector<std::vector<int8_t> > scan_sw_shuf2;
    bool if_filter_l;
    ap_uint<512>* table_join_cfg;
    ap_uint<512>* table_part_cfg;
    void CHECK_0(std::vector<std::string> str, size_t len, std::string sinfo);
    // get join_keys
    void extractKeys(std::string join_str);
    void extractEvals(std::string eval_str, std::vector<std::string> col_names);
    void extractWcols(std::string outputs);
    std::vector<std::string> evalStrRep(std::initializer_list<std::string> evals);
    // for filter
    void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
    ErrCode shuffle(const int tab_tab,
                    std::string& filter_str,
                    std::vector<std::string>& col_names,
                    bool part_flag = false,
                    std::vector<std::string> strs = {"a", "b", "c", "d"},
                    bool replace = true);

    int findJoinInd(std::vector<std::string> join_str, std::string ss);
    // for join
    std::vector<int8_t> shuffle(std::vector<std::string> join_str, std::vector<std::string>& col_names);
    void compressCol(std::vector<int8_t>& shuffle,
                     std::vector<std::string>& col_names,
                     std::vector<std::string> ref_cols);
    // for eval
    std::vector<int8_t> shuffle(size_t eval_id,
                                std::string& eval_str,
                                std::vector<std::string>& col_names,
                                bool replace = true,
                                std::vector<std::string> strs = {"strm1", "strm2", "strm3", "strm4"});

   public:
    /**
     * @brief construct of JoinConfig.
     *
     * The class generate join configure bits by column names,
     *
     * Input filter_a/filter_b like "19940101<=o_orderdate && o_orderdate<19950101",
     * o_orderdate and o_orderdate must be exsisted colunm names in table a/b
     * when no filter conditions, input ""
     *
     * Input all evaluation expressions/evaluation constants in initializer_list,
     * each expression most contains four columns and four constants (use c1,c2,c3,c4 as notations)
     * column names and constants are positional, that is the column name must be left operands, so
     * you can input "-l_extendedprice + c1", but not "c1-l_extendedprice"
     * complete evals and evals_const examples:
     * "-l_extendedprice * (l_discount + 1)":
     * {"-l_extendedprice * (l_discount + c1)"} {{0,1}}
     * "-l_extendedprice * (l_discount + 1) * l_tax":
     * {"-l_extendedprice * (l_discount + c1) * l_tax"} {{0,1,0}}
     *
     * Input join conditions like "left_join_key_0=right_join_key_0"
     * when use dual key join, use comma as seperator,
     * "left_join_key_0=right_join_key_0,left_join_key_1=right_join_key_1"
     *
     * Output strings are like "output_c0 = tab_a_col/tab_b_col",
     * when contains several columns, use comma as seperator
     *
     * Usage:
     *
     * \rst
     * ::
     *
     * JoinConfig  jcmd(tab_a, "19940101<=o_orderdate && o_orderdate<19950101",
     *                  tab_b, "",
     *                  "o_orderkey = l_orderkey",
     *                  {"-l_extendedprice * (l_discount + c1)"}, {{0,1}},
     *                  tab_c, "c1=l_orderkey, c2 = eval0");
     * \endrst
     *
     * @param a left table
     * @param filter_a filter condition of left table
     * @param b right table
     * @param filter_b filter condition of right table
     * @param join_str join condition(s)
     * @param evals eval expressions list
     * @param evals_const eval expression constant list
     * @param c result table
     * @param output_str output column mapping
     * @param join_type INNER_JOIN(default) | SEMI_JOIN | ANTI_JOIN.
     * @param part_tag if use partition kernel
     *
     */
    JoinConfig(Table a,
               std::string filter_a,
               Table b,
               std::string filter_b,
               std::string join_str, // comma separated
               std::initializer_list<std::string> evals,
               std::initializer_list<std::initializer_list<int> > evals_const,
               Table c,
               std::string output_str, // comma separated
               int join_Type = INNER_JOIN,
               bool part_tag = false);

    /**
     * @brief return join config bits.
     *
     * @return join config bits
     */

    ap_uint<512>* getJoinConfigBits() const;

    /**
     * @brief return part config bits.
     *
     * @return partition config bits
     */
    ap_uint<512>* getPartConfigBits() const;
    /**
     * @brief software shuffle list.
     *
     * @return software shuffle array to adjust the kernel input
     */
    std::vector<std::vector<int8_t> > getScanSwShuf1() const;
    /**
     * @brief software shuffle list.
     *
     * @return join software shuffle array in partition+join solution
     */
    std::vector<std::vector<int8_t> > getScanSwShuf2() const;
    /**
     * @brief return if left table in case has filter condition.
     *
     * @return if left table in case has filter condition
     */
    bool getIfFilterL() const;
};
}
}
}
#endif
