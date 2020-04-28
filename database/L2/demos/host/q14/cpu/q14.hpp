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
#include <regex>
// t1 //t6
void PartFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type = tin.getcharN<char, TPCH_READ_P_TYPE_LEN + 1>(i, 1);
        // if(std::regex_match(p_type.data(), std::regex("(PROMO)(.*)"))){
        if (std::string(p_type.data()).find("PROMO") == 0) {
            int32_t p_partkey = tin.getInt32(i, 0);
            tout.setInt32(r, 0, p_partkey);

            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In PartFilter" << std::endl;
}

// t2
int64_t q14Join_t1_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    std::cout << std::dec << nrow1 << " " << ht1.size() << " 0In PartFilter" << std::endl;
    int64_t sum = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_shipdate = tin2.getInt32(i, 3);
        if (l_shipdate >= 19950901 && l_shipdate < 19951001) {
            int32_t l_partkey = tin2.getInt32(i, 0);
            auto it = ht1.find(l_partkey);
            if (it != ht1.end()) {
                int32_t l_extendedprice = tin2.getInt32(i, 1);
                int32_t l_discount = tin2.getInt32(i, 2);
                int32_t e = l_extendedprice * (100 - l_discount);
                sum += e;
            }
        }
    }
    return sum;
}

int64_t q14Join_p_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    std::cout << std::dec << ht1.size() << " In PartFilter" << std::endl;
    int64_t sum = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_shipdate = tin2.getInt32(i, 3);
        if (l_shipdate >= 19950901 && l_shipdate < 19951001) {
            int32_t l_partkey = tin2.getInt32(i, 0);
            auto it = ht1.find(l_partkey);
            if (it != ht1.end()) {
                int32_t l_extendedprice = tin2.getInt32(i, 1);
                int32_t l_discount = tin2.getInt32(i, 2);
                int32_t e = l_extendedprice * (100 - l_discount);
                sum += e;
            }
        }
    }
    return sum;
}
// t3
// q14Join_p_l/q14Join_t1_l
