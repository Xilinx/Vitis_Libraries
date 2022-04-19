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

#ifndef GQE_KERNEL_COMMAND_HPP
#define GQE_KERNEL_COMMAND_HPP

#include "ap_int.h"
// L1
#include "xf_database/enums.hpp"
#include "xf_database/filter_config.hpp"
#include "xf_database/dynamic_alu_host.hpp"
#include "xf_database/dynamic_eval_v2_config.hpp"
// L2
#include "xf_database/gqe_utils.hpp"

//#define USER_DEBUG 1

namespace xf {
namespace database {

enum JoinType { INNER_JOIN = 0, SEMMI_JOIN = 1, ANTI_JOIN = 2 };

namespace gqe {

/**
 * @class KernelCommand kernel_command.hpp "xf_database/kernel_command.hpp"
 * @brief Class for generating configuration bits of ``gqePart``, ``gqeJoin`` and  ``gqeFilter`` kernels.
 */
class KernelCommand {
   private:
    ap_uint<512>* hbuf;
    int joinType;
    bool isBypassOn;
    bool isJoinOn;
    bool isBloomfilterOn;
    bool isPartOn;
    bool isAggrOn;
    bool isDualKeyOn;

   public:
    /**
    * @brief constructor of KernelCommand class. The kernel command used buffer is allocated on host. Some default
    * settings are provided.
    */
    KernelCommand() {
        hbuf = gqe::utils::aligned_alloc<ap_uint<512> >(14);
        memset(hbuf, 0, sizeof(ap_uint<512>) * 14);
        hbuf[9].range(159, 159) = 1;
        hbuf[13].range(159, 159) = 1;
        isBypassOn = false;
        isJoinOn = false;
        isBloomfilterOn = false;
        isPartOn = false;
        isAggrOn = false;
        isDualKeyOn = false;
        joinType = 0;
    }
    ~KernelCommand() { free(hbuf); }

    /**
     * @brief set bypass on.
     *
     * @param trigger 0 for off, 1 for on.
     *
     */
    void setBypassOn(bool trigger) {
        isBypassOn = trigger;
        hbuf[0][0] = trigger;
    }
    /**
     * @brief set join on.
     *
     * @param trigger 0 for off, 1 for on.
     *
     */
    void setJoinOn(bool trigger) {
        isJoinOn = trigger;
        hbuf[0][1] = trigger;
    }

    /**
     * @brief set join type.
     *
     * @param join_type hash join type, 0 for INNER, 1 for SEMI, 2 for ANTI.
     *
     */
    void setJoinType(int join_type) {
        if (join_type > 2) {
            std::cerr << "Error: only INNER/SEMI/ANTI join supported." << std::endl;
            exit(1);
        }
        hbuf[1].range(51, 50) = join_type;
    }

    /**
     * @brief set join append mode.
     *
     * @param ap_mode append mode.
     *
     */
    void setJoinAppendMode(int ap_mode) {
        std::cout << "Warning: append mode is not supported in GQE currently. Ignored user-specified append mode."
                  << std::endl;
        hbuf[1][52] = ap_mode;
    }

    /**
     * @brief set bloom-filter on.
     *
     * @param bf_size size of bloom-filter in bits, we need 35 bits to represent a range between 1 to 16 Gbits.
     *
     */
    void setBloomfilterOn(bool trigger) {
        isBloomfilterOn = trigger;
        hbuf[0][2] = trigger;
    }

    /**
     * @brief set bloom-filter size.
     *
     * @param bf_size size of bloom-filter in bits, we need 35 bits to represent a range between 1 to 16 Gbits.
     *
     */
    void setBloomfilterSize(ap_uint<36> bf_size) {
        // As we use 16 HBMs and 512-bit cache line size
        // The size of the bloom filter should be 16 * 512 bits aligned
        if ((bf_size % 8192) != 0) {
            std::cout << "Error: size of bloom filter have to be 8192-bit aligned\n";
            exit(1);
        }
        // Current hardware storage limitation
        if (bf_size > 32UL * 1024 * 1024 * 1024) {
            std::cout << "Error: maximum supported bloom filter size is 32Gbits\n";
            exit(1);
        }
        hbuf[3].range(85, 50) = bf_size;
    }

    /**
     * @brief set partition on.
     *
     * @param trigger 0 for off, 1 for on.
     *
     */
    void setPartOn(int trigger) {
        isPartOn = trigger;
        hbuf[0][3] = trigger;
    }

    /**
     * @brief set log of partition number.
     *
     * @param log_part log of partition number.
     *
     */
    void setLogPart(int log_part) {
        if (log_part < 3) {
            std::cerr << "Error, supported minimum number of log_part is 3" << std::endl;
            exit(1);
        }
        if (log_part > 8) {
            std::cerr << "Error, supported maximum number of log_part is 8" << std::endl;
        }
        // set log part
        hbuf[2].range(53, 50) = log_part;
    }

    // TODO: add required parameters when designing 4-in-1 GQE.
    /**
     * @brief set aggregate on.
     *
     * @param trigger false for off, true for on.
     */
    void setAggrOn(bool trigger = true) {
        isAggrOn = trigger;
        hbuf[0][4] = trigger;
    }

    /**
     * @brief set dual key on.
     *
     * @param trigger false for using 1 column as key, true for using two.
     *
     */
    void setDualKeyOn(bool trigger = true) {
        isDualKeyOn = trigger;
        hbuf[0][6] = trigger;
    }

    /**
     * @brief set join build probe flag.
     *
     * @param flag 0 for build, 1 for probe.
     *
     */
    void setJoinBuildProbe(bool flag) { hbuf[0][5] = flag; }

    /**
     * @brief set bloom filter build probe flag.
     *
     * @param flag 0 for build, 1 for probe.
     *
     */
    void setBloomfilterBuildProbe(bool flag) { hbuf[0][7] = flag; }

    /**
     * @brief enables input columns for gqeKernel.
     *
     * @param table_id 0 for left table, 1 for right table.
     * @param index valid input column ids.
     *
     */
    void setScanColEnable(int table_id, std::vector<int8_t> index) {
        if (table_id > 1) {
            std::cerr << "Error: table_id can only be set to 0 or 1" << std::endl;
            exit(1);
        }
        // table A
        if (table_id == 0) {
            for (size_t d = 0; d < index.size(); d++) {
                if (index[d] != -1) {
                    hbuf[0].set_bit((10 + index[d]), 1);
                }
            }
            // table B
        } else {
            for (size_t d = 0; d < index.size(); d++) {
                if (index[d] != -1) {
                    hbuf[0].set_bit((13 + index[d]), 1);
                }
            }
        }
#ifdef USER_DEBUG
        if (table_id == 0)
            std::cout << "table A enable col / hbuf[0].range(12, 10): " << hbuf[0].range(12, 10) << std::endl;
        else
            std::cout << "table B enable col / hbuf[0].range(15, 13): " << hbuf[0].range(15, 13) << std::endl;
#endif
    }

    /**
     * @brief enables output columns for gqeKernel.
     *
     * @param krn_type 0 for gqeJoin, 1 for gqePart, 2 for gqeFilter
     * @param table_id 0 for left table, 1 for right table
     * @param index output column ids
     *
     */
    void setWriteColEnable(int krn_type, int table_id, std::vector<int8_t> index) {
        int8_t value = 0;
        // gqeJoin / gqeFilter setup
        if (krn_type == 0 || krn_type == 2) {
            if (index.size() > 4) {
                std::cerr << "Error: max supported number of columns is 4" << std::endl;
                exit(1);
            }
            for (size_t i = 0; i < index.size(); i++) {
                if (index[i] >= 0) value += (1 << index[i]);
            }
            if (table_id) {
                hbuf[0].range(23, 20) = value;
            } else {
                hbuf[0].range(19, 16) = value;
            }
            // gqePart setup
        } else {
            if (index.size() > 3) {
                std::cerr << "Error: max supported number of columns is 3" << std::endl;
                exit(1);
            }
            for (size_t i = 0; i < index.size(); i++) {
                if (index[i] >= 0) value += (1 << index[i]);
            }
            if (table_id) {
                // L table
                hbuf[0].range(22, 20) = value;
            } else {
                // O table
                hbuf[0].range(18, 16) = value;
            }
        }
    }

    /**
     * @brief set gen_rowIDEnable and validEnable flag.
     *
     * @param table_id 0 for left table, 1 for right table.
     * @param gen_rowID_en enable flag for using GQE to generate row IDs internally. 1 for enable, 0 for disable.
     * @param valid_en enable flag for getting valid bits from off-chip memory or enabing every row internally. 1 for
     * valid bits from off-chip memory, 0 for enabling every row.
     *
     */
    void setRowIDValidEnable(int table_id, bool gen_rowID_en, bool valid_en) {
        // gen-rowid for tab
        hbuf[0].set_bit(30 + 2 * table_id, gen_rowID_en);
        // valid_en for tab
        hbuf[0].set_bit(31 + 2 * table_id, valid_en);
    }

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
     * @param table_id 0 for left table, 1 for right table
     * @param filter_string filter expression string
     *
     */
    void setFilter(int table_id, std::string filter_string) {
        FilterConfig<64> fp(filter_string);
        auto cfg = fp.getConfigBits();
        memcpy(&hbuf[table_id * 4 + 6], cfg.get(), sizeof(uint32_t) * 53);
#ifdef USER_DEBUG
        std::cout << "setFilter cfg---------" << std::endl;
        for (int i = 0; i < 53; i++) {
            std::cout << cfg[i] << std::endl;
        }
        std::cout << "setFilter cfg end ---------" << std::endl;
#endif
    }

    /**
     * @brief return config bit array.
     *
     */
    ap_uint<512>* getConfigBits() const { return hbuf; }

#ifdef USER_DEBUG
    /**
     * @brief for comparing the current configurations with the original ones.
     *
     * @param cmpbuf input configurations to be compared.
     * @param len length of the configurations, currently we have 14 * 512-bits size of kernel configurations.
    */
    void cmpConfig(ap_uint<512>* cmpbuf, int len = 14) {
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

#endif // GQE_KERNEL_COMMAND_HPP
