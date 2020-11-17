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
#ifndef _GQE_JOIN_STRATEGY_L3_
#define _GQE_JOIN_STRATEGY_L3_

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <cstdio>
// L3
#include "xf_database/gqe_table.hpp"

namespace xf {
namespace database {
namespace gqe {

enum DATASIZE : int64_t {
    sz_1g = 1024 * 1024 * 1024,
    sz_2g = 2 * sz_1g,
    sz_4g = 4 * sz_1g,
    sz_10g = 10 * sz_1g,
    sz_20g = 20 * sz_1g,
    sz_100g = 100 * sz_1g,
    sz_200g = 200 * sz_1g,
    sz_1000g = 1000 * sz_1g,
    sf1_o_n = 1500000,
    sf1_l_n = 6001215,
    sf8_o_n = 8 * 1500000,
    sf8_l_n = 8 * 6001215,
    sf20_o_n = 20 * 1500000,
    sf20_l_n = 20 * 6001215
};

enum SOLUTION { SOL0 = 0, SOL1 = 1, SOL2 = 2 };

class JoinStrategyBase {
   public:
    /**
     * @brief construct of JoinStrategyBase.
     *
     */
    JoinStrategyBase(){};
    virtual ~JoinStrategyBase(){};

    /**
     * @brief get solution id and parameters.
     *
     * @param tab_a left table
     * @param tab_b right table
     *
     */
    virtual std::vector<size_t> getSolutionParams(Table tab_a, Table tab_b) { return {0, 0, 0, 0, 0, 0}; };
    //
};

//--------------------------------------------------------------------------------//
// Derived Classes
//--------------------------------------------------------------------------------//

class JoinStrategyV1 : public JoinStrategyBase {
   public:
    /**
     * @brief construct of JoinStrategyV1.
     *
     */
    JoinStrategyV1(){};
    ~JoinStrategyV1(){};

    /**
     * @brief get solution id and parameters.
     *
     * @param tab_a left table
     * @param tab_b right table
     *
     */
    std::vector<size_t> getSolutionParams(Table tab_a, Table tab_b) {
        // int64_t tb0_one_col_sz = tb0_n * type_size_tb0;
        // int64_t tb1_one_col_sz = tb1_n * type_size_tb1;
        size_t tb0_n = tab_a.getRowNum();
        size_t tb1_n = tab_b.getRowNum();
        size_t type_size_tb0 = sizeof(int32_t);
        size_t type_size_tb1 = sizeof(int32_t);
        size_t valid_col_num_tb0 = tab_a.getColNum();
        size_t valid_col_num_tb1 = tab_b.getColNum();

        int64_t tb0_sz = valid_col_num_tb0 * tb0_n * type_size_tb0;
        int64_t tb1_sz = valid_col_num_tb1 * tb1_n * type_size_tb1;
        std::cout << "tab 0 data size: " << (double)tb0_sz / 1024 / 1024 << " MB" << std::endl;
        std::cout << "tab 1 data size: " << (double)tb1_sz / 1024 / 1024 << " MB" << std::endl;

        size_t _solution;
        size_t sec_o;
        size_t sec_l;
        size_t slice_num;
        size_t log_part;
        // when sf1
        if (tb0_n <= sf1_o_n && tb1_n <= sf1_l_n) {
            _solution = 0;
            // not wrok
            slice_num = 1;
            sec_o = 1;
            sec_l = 1;
            log_part = 0;
        } else if (tb0_n <= sf20_o_n) { // when left table  < sf20, we will compare the perf between sol1 and sol2
            if (valid_col_num_tb0 <= 3 &&
                valid_col_num_tb1 <= 3) { // partition maybe gets better perf because partition kernel has its best perf
                if (tb0_n <= sf8_o_n && tb1_n <= sf8_l_n) { // solution 1 is better
                    _solution = 1;
                    slice_num = tb1_n / sf1_l_n;
                    // not wrok
                    sec_o = 1;
                    sec_l = 1;
                    log_part = 0;
                } else { // solution 2 is better
                    _solution = 2;
                    slice_num = 1;
                    sec_o = tb0_n / sf1_o_n;
                    log_part = std::log(sec_o) / std::log(2);
                }
            } else {
                _solution = 1;
                slice_num = tb1_n / sf1_l_n;
                // not wrok
                sec_o = 1;
                sec_l = 1;
                log_part = 0;
            }
        } else {
            _solution = 2;
            slice_num = 1;
            int32_t sec_o_1 = tb0_sz / sz_1g;
            int32_t sec_o_2 = tb0_n / sf1_o_n;
            sec_o = (sec_o_1 > sec_o_2) ? sec_o_1 : sec_o_2;
            log_part = std::log(sec_o) / std::log(2);
        }

#ifdef USER_DEBUG
        std::cout << "create gqe::SOL" << _solution << std::endl;
        std::cout << "sec_l:" << sec_o << ", sec_r:" << sec_l << ", log_part:" << log_part
                  << ", slice_num:" << slice_num << std::endl;
#endif
        return {_solution, sec_o, sec_l, slice_num, log_part, true};
    }
};

class JoinStrategyManualSet : public JoinStrategyBase {
   private:
    size_t sol;
    size_t sec_l;
    size_t sec_r;
    size_t slice_num;
    size_t log_part;
    bool probe_buf_size_auto;

   public:
    JoinStrategyManualSet(){};
    ~JoinStrategyManualSet(){};

    /**
     * @brief construct of JoinStrategyManualSet.
     *
     * derived class of JoinStrategyBase, for set solution and parameters manually
     *
     * @param sol solution id SOL0 | SOL1 | SOL2.
     * @param sec_l section number of left table
     * @param sec_r section number of right table
     * @param slice_num slice number of probe kernel.
     * @param log_part log number of hash partition.
     * @param _probe_buf_size_auto solution2: when true (default), auto set, false, refer to size of user buffer
     * //todo:  current version, only solution2 add cpu aggregation, reserve the parameter
     *
     */
    JoinStrategyManualSet(size_t _sol,
                          size_t _sec_l,
                          size_t _sec_r,
                          size_t _slice_num,
                          size_t _log_part,
                          bool _probe_buf_size_auto = true) {
        sol = _sol;
        sec_l = _sec_l;
        sec_r = _sec_r;
        slice_num = _slice_num;
        log_part = _log_part;
        probe_buf_size_auto = _probe_buf_size_auto;
    };

    /**
     * @brief get solution id and parameters.
     *
     * @param tab_a left table
     * @param tab_b right table
     *
     */
    std::vector<size_t> getSolutionParams(Table tab_a, Table tab_b) {
#ifdef USER_DEBUG
        std::cout << "create gqe::SOL" << sol << std::endl;
        std::cout << "sec_l:" << sec_l << ", sec_r:" << sec_r << ", log_part:" << log_part
                  << ", slice_num:" << slice_num << std::endl;
#endif
        return {sol, sec_l, sec_r, slice_num, log_part, probe_buf_size_auto};
    };
};

} // gqe
} // database
} // xf
#endif
