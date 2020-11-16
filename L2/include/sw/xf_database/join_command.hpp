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

#ifndef GQE_JOIN_COMMAND_HPP
#define GQE_JOIN_COMMAND_HPP

#include "ap_int.h"
// L1
#include "xf_database/enums.hpp"
#include "xf_database/filter_config.hpp"
#include "xf_database/dynamic_alu_host.hpp"
#include "xf_database/dynamic_eval_v2_config.hpp"
// L2
#include "xf_database/gqe_utils.hpp"

namespace xf {
namespace database {

enum JoinType { INNER_JOIN = 0, SEMMI_JOIN = 1, ANTI_JOIN = 2 };

namespace gqe {

/**
 * @class JoinCommand join_command.hpp "xf_database/join_command.hpp"
 * @brief Class for generating configuration bits of ``gqeJoin`` kernel.
 */
class JoinCommand {
   private:
    ap_uint<512>* hbuf;
    int joinType;
    bool isBypass;
    bool isAggrOn;
    bool isDualKeyOn;

    void setIndElems(int offset, std::vector<int8_t> index, int tag = 0) {
        for (unsigned c = 0; c < index.size(); ++c) {
            hbuf[tag].range(offset + 8 * c + 7, offset + 8 * c) = index[c];
        }
        for (unsigned c = index.size(); c < 8; c++) {
            hbuf[tag].range(offset + 8 * c + 7, offset + 8 * c) = -1;
        }
    }

   public:
    /**
     * @brief construct of JoinCommand.
     *
     * default:
     *         join         : on
     *         join type    : INNER_JOIN
     *         bypass       : off
     *         aggr         : off
     *         dual key     : off
     *
     */

    JoinCommand() {
        hbuf = gqe::utils::aligned_alloc<ap_uint<512> >(9);
        memset(hbuf, 0, sizeof(ap_uint<512>) * 9);
        hbuf[0] = 1;
        hbuf[5].range(415, 415) = 1;
        hbuf[8].range(415, 415) = 1;
        isBypass = false;
        isAggrOn = false;
        isDualKeyOn = false;
    }
    ~JoinCommand() { free(hbuf); }

    /**
     * @brief scan valid cols.
     *
     * @param table_id 0 for left table, 1 for right table
     * @param index, valid input column ids
     *
     */

    void Scan(int table_id, std::vector<int8_t> index) {
        if (table_id > 1) {
            std::cerr << "table_id can only be set 0/1" << std::endl;
            exit(1);
        }
        int offset = table_id * 64 + 56;
        setIndElems(offset, index);
    }

    /*
     *
     *1. hbuf[0]
     *
     * */

    /**
     * @brief set bypass on.
     *
     */
    void setBypassOn() {
        isBypass = true;
        hbuf[0].set_bit(0, 0);
    }

    /**
     * @brief set aggregation on.
     *
     * not support in current join kernel
     *
     */
    void setAggrOn() {
        std::cerr << "Not support function:setAggrOn(), please set aggr off!" << std::endl;
        exit(1);
        isAggrOn = true;
        hbuf[0].set_bit(1, 1);
    }

    /**
     * @brief set dual key join on.
     *
     */
    void setDualKeyOn() {
        isDualKeyOn = true;
        hbuf[0].set_bit(2, 1);
    }

    /**
     * @brief set join type.
     *
     * @param jointype Join type, default is INNER_JOIN
     */

    void setJoinType(int jointype) {
        joinType = jointype;
        hbuf[0].range(5, 3) = jointype;
    }

    /**
     * @brief set output column ids.
     *
     * @param index output column ids
     *
     */
    void setWriteCol(std::vector<int8_t> index) {
        int8_t value = 0;
        if (index.size() > 8) {
            std::cerr << "Error,max supporting col number is 8" << std::endl;
            exit(1);
        }
        for (size_t i = 0; i < index.size(); i++) {
            value += (1 << index[i]);
        }
        hbuf[0].range(191, 184) = value;
    }

    /**
     * @brief set shuffle0 column ids.
     *
     *
     * @param table_id 0 for left table, 1 for right table
     * @param index shuffle0 key list, if filter on, move filter keys to first 4 locations
     *
     */

    void setShuffle0(int table_id, std::vector<int8_t> index) {
        if (table_id > 1) {
            std::cerr << "table_id can only be set 0/1" << std::endl;
            exit(1);
        }
        setIndElems(448 - 64 * table_id, index, 1);
    }

    // setShuffle0_a and  setShuffle0_b are merged in setShuffle0
    void setShuffle0_a(std::vector<int8_t> index) { setIndElems(448, index, 1); }
    void setShuffle0_b(std::vector<int8_t> index) { setIndElems(384, index, 1); }

    /**
     * @brief set shuffle1 column ids.
     *
     *
     * @param table_id 0 for left table, 1 for right table
     * @param index shuffle1 key list, if join on, move join key(s) to first 1 (2 when dual key) location(s)
     *
     */
    void setShuffle1(int table_id, std::vector<int8_t> index) {
        if (table_id > 1) {
            std::cerr << "table_id can only be set 0/1" << std::endl;
            exit(1);
        }
        setIndElems(192 + 64 * table_id, index);
    }

    // setShuffle1_a and  setShuffle1_b are merged in setShuffle1
    void setShuffle1_a(std::vector<int8_t> index) { setIndElems(192, index); }
    void setShuffle1_b(std::vector<int8_t> index) { setIndElems(256, index); }

    /**
     * @brief set shuffle2 column ids.
     *
     * @param index shuffle2 key list, for evaluation 1, move evaluation keys to first 4 locations
     *
     */
    void setShuffle2(std::vector<int8_t> index) { setIndElems(320, index); }

    /**
     * @brief set shuffle3 column ids.
     *
     * @param index shuffle3 key list, for evaluation 2, move evaluation keys to first 4 locations
     *
     */
    void setShuffle3(std::vector<int8_t> index) { setIndElems(384, index); }

    /**
     * @brief set shuffle4 column ids.
     *
     * @param index aggregation key list
     *
     */
    void setShuffle4(std::vector<int8_t> index) {
        // if (!isAggrOn) {
        //    std::cerr << "WARNING: please set AggrOn first." << std::endl;
        //}
        setIndElems(448, index);
    }

    /*
     *
     * 2. hbuf[0x01~0x02]
     *
     * */

    /**
     * @brief set Evaluation string.
     *
     * @param eval_id, most two evaluations
     * @param s0, evaluation string for special strm format
     * @param index, evaluation constants for each strm, default 0
     *
     */
    void setEvaluation(int eval_id, std::string s0, std::vector<int32_t> index) {
        ap_uint<289> op = 0;
        std::vector<int32_t> conts(4);
        size_t c = 0;
        for (c = 0; c < index.size(); c++) {
            conts[c] = index[c];
        }
        while (c < 4) {
            conts[c] = 0;
            c++;
        }
        xf::database::dynamicALUOPCompiler<uint32_t, uint32_t, uint32_t, uint32_t>(s0.c_str(), conts[0], conts[1],
                                                                                   conts[2], conts[3], op);
        hbuf[eval_id + 1] = op;
    }

    // TODO update
    /**
     * @brief set Evaluation string.
     *
     * @param eval_id, most two evaluations
     * @param eval_str, evaluation expression string
     *
     */
    void setEvaluation(int eval_id, std::string eval_str) {
        if (eval_id > 1) {
            std::cerr << "eval_id can only be set 0/1" << std::endl;
            exit(1);
        }
        auto eval_bits = DynamicEvalV2Config(eval_str).getConfigBits();
        memcpy(&hbuf[eval_id + 1], eval_bits.get(), sizeof(uint32_t) * 8);
    }
    /*
     *
     * 3. hbuf[0x03~0x08]
     *
     * */
    /**
     * @brief set Filter string.
     *
     * The string uses ``a``, ``b``, ``c``, ``d`` to refer to first to the fourth column,
     * and supports comparison and logical operator like C++.
     * Parentheses can be used to group logic, but notice that the underlying hardware module
     * does not support comparing one column with multiple constant boundaries in OR relationship.
     * Integral constants will be extracted from expression.
     *
     * For example, an expression could be ``(a < 10 && b < 20) || (c >= d)``.
     *
     * @param table_id, 0 for left table, 1 for right table
     * @param filter_string filter expression string
     *
     */
    void setFilter(int table_id, std::string filter_string) {
        FilterConfig fp(filter_string);
        auto cfg = fp.getConfigBits();
        memcpy(&hbuf[(table_id + 1) * 3], cfg.get(), sizeof(uint32_t) * 45);
    }

    // TODO deprecate
    void setFilter(int table_id, void (*get_filter_func)(uint32_t cfg[])) {
        uint32_t cfg[45];
        get_filter_func(cfg);
        memcpy(&hbuf[table_id * 3 + 3], cfg, sizeof(uint32_t) * 45);
    }

    /**
     * @brief return config bit array.
     *
     */
    ap_uint<512>* getConfigBits() const { return hbuf; }

#ifdef USER_DEBUG
    /*
        std::shared_ptr<ap_uint<512> > getConfigBits() const {
            std::shared_ptr<ap_uint<512> > cfgs(hbuf);
            return cfgs;
        }
    */
    // for testing with the original cfg
    void cmpConfig(ap_uint<512>* cmpbuf, int len = 9) {
        int difnum = 0;
        for (int i = 0; i < len; i++) {
            ap_uint<512> l = hbuf[i];
            ap_uint<512> r = cmpbuf[i];
            ap_uint<512> l_xor_r = l ^ r;
            if (l_xor_r != 0) {
                for (int k = 0; k < 512; k++) {
                    difnum += ((l_xor_r >> k) & 0x01);
                    if (((l_xor_r >> k) & 0x01)) {
                        std::cout << k << " ";
                    }
                }
                std::cout << "hbufs differ in " << i << " row" << std::endl;
            }
        }
        if (!difnum) {
            std::cout << "Test Right!" << std::endl;
        } else {
            std::cout << "Total " << difnum << " elems" << std::endl;
        }
    }
#endif
};

} // namespace gqe
} // namespace database
} // namespace xf

#endif // GQE_JOIN_COMMAND_HPP
