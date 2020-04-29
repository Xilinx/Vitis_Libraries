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
// t1
void PartFilter1(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 1);
        std::array<char, TPCH_READ_P_CNTR_LEN + 1> p_container = tin.getcharN<char, TPCH_READ_P_CNTR_LEN + 1>(i, 2);
        if ((std::string(p_container.data()) == "SM CASE" || std::string(p_container.data()) == "SM BOX" ||
             std::string(p_container.data()) == "SM PACK" || std::string(p_container.data()) == "SM PKG") &&
            std::string(p_brand.data()) == "Brand#12") {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout.setInt32(r, 0, p_partkey);
            tout.setInt32(r, 1, p_size);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after PartFilter1" << std::endl;
}
void PartFilter_(Table& tin, Table& tout0, Table& tout1, Table& tout2) {
    int nrow = tin.getNumRow();
    int r0 = 0;
    int r1 = 0;
    int r2 = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 1);
        std::array<char, TPCH_READ_P_CNTR_LEN + 1> p_container = tin.getcharN<char, TPCH_READ_P_CNTR_LEN + 1>(i, 2);
        if (std::string(p_brand.data()) == "Brand#12" &&
            (std::string(p_container.data()) == "SM CASE" || std::string(p_container.data()) == "SM BOX" ||
             std::string(p_container.data()) == "SM PACK" || std::string(p_container.data()) == "SM PKG")) {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout0.setInt32(r0, 0, p_partkey);
            tout0.setInt32(r0, 1, p_size);
            r0++;
        } else if (std::string(p_brand.data()) == "Brand#23" &&
                   (std::string(p_container.data()) == "MED BAG" || std::string(p_container.data()) == "MED BOX" ||
                    std::string(p_container.data()) == "MED PKG" || std::string(p_container.data()) == "MED PACK")) {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout1.setInt32(r1, 0, p_partkey);
            tout1.setInt32(r1, 1, p_size);
            r1++;
        } else if (std::string(p_brand.data()) == "Brand#34" &&
                   (std::string(p_container.data()) == "LG CASE" || std::string(p_container.data()) == "LG BOX" ||
                    std::string(p_container.data()) == "LG PACK" || std::string(p_container.data()) == "LG PKG")) {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout2.setInt32(r2, 0, p_partkey);
            tout2.setInt32(r2, 1, p_size);
            r2++;
        }
    }
    tout0.setNumRow(r0);
    tout1.setNumRow(r1);
    tout2.setNumRow(r2);
    std::cout << std::dec << r0 << " after_ PartFilter1" << std::endl;
    std::cout << std::dec << r1 << " after_ PartFilter2" << std::endl;
    std::cout << std::dec << r2 << " after_ PartFilter3" << std::endl;
}
void PartFilter(Table& tin, Table& tout0, Table& tout1, Table& tout2) {
    std::unordered_map<std::string, int32_t> ht1;
    std::unordered_map<std::string, int32_t> ht2;
    std::unordered_map<std::string, int32_t> ht3;
    ht1.insert(std::make_pair("SM PACK", 0));
    ht1.insert(std::make_pair("SM CASE", 0));
    ht1.insert(std::make_pair("SM BOX", 0));
    ht1.insert(std::make_pair("SM PKG", 0));
    ht2.insert(std::make_pair("MED BAG", 0));
    ht2.insert(std::make_pair("MED BOX", 0));
    ht2.insert(std::make_pair("MED PKG", 0));
    ht2.insert(std::make_pair("MED PACK", 0));
    ht3.insert(std::make_pair("LG CASE", 0));
    ht3.insert(std::make_pair("LG BOX", 0));
    ht3.insert(std::make_pair("LG PACK", 0));
    ht3.insert(std::make_pair("LG PKG", 0));
    int nrow = tin.getNumRow();
    int r0 = 0;
    int r1 = 0;
    int r2 = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 1);
        std::array<char, TPCH_READ_P_CNTR_LEN + 1> p_container = tin.getcharN<char, TPCH_READ_P_CNTR_LEN + 1>(i, 2);
        if (std::string(p_brand.data()) == "Brand#12" && ht1.find(std::string(p_container.data())) != ht1.end()) {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout0.setInt32(r0, 0, p_partkey);
            tout0.setInt32(r0, 1, p_size);
            r0++;
        } else if (std::string(p_brand.data()) == "Brand#23" &&
                   ht2.find(std::string(p_container.data())) != ht2.end()) {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout1.setInt32(r1, 0, p_partkey);
            tout1.setInt32(r1, 1, p_size);
            r1++;
        } else if (std::string(p_brand.data()) == "Brand#34" &&
                   ht3.find(std::string(p_container.data())) != ht3.end()) {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout2.setInt32(r2, 0, p_partkey);
            tout2.setInt32(r2, 1, p_size);
            r2++;
        }
    }
    tout0.setNumRow(r0);
    tout1.setNumRow(r1);
    tout2.setNumRow(r2);
    std::cout << std::dec << r0 << " after_ PartFilter1" << std::endl;
    std::cout << std::dec << r1 << " after_ PartFilter2" << std::endl;
    std::cout << std::dec << r2 << " after_ PartFilter3" << std::endl;
}
// t2
void LineFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> l_shipmode = tin.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(i, 4);
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> l_shipinstruct = tin.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(i, 5);
        if ((std::string(l_shipmode.data()) == "AIR" || std::string(l_shipmode.data()) == "AIR REG") &&
            std::string(l_shipinstruct.data()) == "DELIVER IN PERSON") {
            int32_t l_partkey = tin.getInt32(i, 0);
            int32_t l_quantity = tin.getInt32(i, 1);
            int32_t l_extendedprice = tin.getInt32(i, 2);
            int32_t l_discount = tin.getInt32(i, 3);
            tout.setInt32(r, 0, l_partkey);
            tout.setInt32(r, 1, l_quantity);
            tout.setInt32(r, 2, l_extendedprice);
            tout.setInt32(r, 3, l_discount);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after LineFilter1" << std::endl;
}
// t3
void q19Join_t1_t2(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int64_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        int32_t p_size = tin1.getInt32(i, 1);
        if (p_size >= 1 && p_size <= 5) {
            ht1.insert(std::make_pair(p_partkey, p_size));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_quantity = tin2.getInt32(i, 1);
        int32_t l_partkey = tin2.getInt32(i, 0);
        if (l_quantity >= 1 && l_quantity <= 11) {
            auto it = ht1.find(l_partkey);
            if (it != ht1.end()) {
                int32_t l_extendedprice = tin2.getInt32(i, 2);
                int32_t l_discount = tin2.getInt32(i, 3);
                int32_t e = l_extendedprice * (100 - l_discount);
                tout.setInt32(r, 0, e);
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q19Join_t1_t2" << std::endl;
}
// t4
void PartFilter2(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 1);
        std::array<char, TPCH_READ_P_CNTR_LEN + 1> p_container = tin.getcharN<char, TPCH_READ_P_CNTR_LEN + 1>(i, 2);
        if ((std::string(p_container.data()) == "MED BAG" || std::string(p_container.data()) == "MED BOX" ||
             std::string(p_container.data()) == "MED PKG" || std::string(p_container.data()) == "MED PACK") &&
            std::string(p_brand.data()) == "Brand#23") {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout.setInt32(r, 0, p_partkey);
            tout.setInt32(r, 1, p_size);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after PartFilter2" << std::endl;
}
// t6
void PartFilter3(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 1);
        std::array<char, TPCH_READ_P_CNTR_LEN + 1> p_container = tin.getcharN<char, TPCH_READ_P_CNTR_LEN + 1>(i, 2);
        if ((std::string(p_container.data()) == "LG CASE" || std::string(p_container.data()) == "LG BOX" ||
             std::string(p_container.data()) == "LG PACK" || std::string(p_container.data()) == "LG PKG") &&
            std::string(p_brand.data()) == "Brand#34") {
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            tout.setInt32(r, 0, p_partkey);
            tout.setInt32(r, 1, p_size);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after PartFilter3" << std::endl;
}
// t5
void q19Join_t4_t2(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int64_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        int32_t p_size = tin1.getInt32(i, 1);
        if (p_size >= 1 && p_size <= 10) {
            ht1.insert(std::make_pair(p_partkey, p_size));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_quantity = tin2.getInt32(i, 1);
        int32_t l_partkey = tin2.getInt32(i, 0);
        if (l_quantity >= 10 && l_quantity <= 20) {
            auto it = ht1.find(l_partkey);
            if (it != ht1.end()) {
                int32_t l_extendedprice = tin2.getInt32(i, 2);
                int32_t l_discount = tin2.getInt32(i, 3);
                int32_t e = l_extendedprice * (100 - l_discount);
                tout.setInt32(r, 0, e);
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q19Join_t4_t2" << std::endl;
}

// t7
void q19Join_t6_t2(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int64_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        int32_t p_size = tin1.getInt32(i, 1);
        if (p_size >= 1 && p_size <= 15) {
            ht1.insert(std::make_pair(p_partkey, p_size));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_quantity = tin2.getInt32(i, 1);
        int32_t l_partkey = tin2.getInt32(i, 0);
        if (l_quantity >= 20 && l_quantity <= 30) {
            auto it = ht1.find(l_partkey);
            if (it != ht1.end()) {
                int32_t l_extendedprice = tin2.getInt32(i, 2);
                int32_t l_discount = tin2.getInt32(i, 3);
                int32_t e = l_extendedprice * (100 - l_discount);
                tout.setInt32(r, 0, e);
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q19Join_t6_t2" << std::endl;
}

// t3 t5 t7
int64_t q19GroupBy(Table& tin, Table& tout) {
    int64_t sum = 0;
    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int32_t e = tin.getInt32(i, 0);
        sum += e;
    }
    return sum;
    std::cout << std::dec << sum << " after q19Join_t6_t2" << std::endl;
}
