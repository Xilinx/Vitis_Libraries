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
// 2388
void PartFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_NAME_LEN + 1> p_name = tin.getcharN<char, TPCH_READ_P_NAME_LEN + 1>(i, 1);
        // if(std::regex_match(p_name.data(), std::regex("(forest)(.*)"))){
        if (std::string(p_name.data()).find("forest") == 0) {
            int32_t p_partkey = tin.getInt32(i, 0);
            tout.setInt32(r, 0, p_partkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after PartFilter" << std::endl;
}

void q20Join_t1_p(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        ht1.insert(std::make_pair(p_partkey, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_partkey = tin2.getInt32(i, 0);
        auto it = ht1.find(ps_partkey);
        if (it != ht1.end()) {
            int32_t ps_suppkey = tin2.getInt32(i, 1);
            int32_t ps_availqty = tin2.getInt32(i, 2);
            tout.setInt32(r, 0, ps_partkey);
            tout.setInt32(r, 1, ps_suppkey);
            tout.setInt32(r, 2, ps_availqty);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q20Join_t1_p" << std::endl;
}
struct Q20GroupKey {
    int32_t l_partkey;
    int32_t l_suppkey;
    bool operator==(const Q20GroupKey& other) const {
        return (l_partkey == other.l_partkey) && (l_suppkey == other.l_suppkey);
    }
};
namespace std {
template <>
struct hash<Q20GroupKey> {
    std::size_t operator()(const Q20GroupKey& k) const {
        using std::size_t;
        using std::hash;
        return (hash<int>()(k.l_partkey)) + (hash<int>()(k.l_suppkey));
    }
};
}
void q20GroupBy(Table& tin, Table& tout) {
    std::unordered_map<Q20GroupKey, int64_t> m;
    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int32_t l_partkey = tin.getInt32(i, 0);
        int32_t l_suppkey = tin.getInt32(i, 1);
        int32_t l_shipdate = tin.getInt32(i, 2);
        int32_t l_quantity = tin.getInt32(i, 3);
        if (l_shipdate >= 19940101 && l_shipdate < 19950101) {
            Q20GroupKey k{l_partkey, l_suppkey};
            auto it = m.find(k);
            if (it != m.end()) {
                int64_t t = it->second + l_quantity;
                m[k] = t;
            } else {
                m.insert(std::make_pair(k, l_quantity));
            }
        }
    }
    int r = 0;
    for (auto& it : m) {
        tout.setInt32(r, 0, it.first.l_partkey);
        tout.setInt32(r, 1, it.first.l_suppkey);
        int64_t sumall = 5 * it.second;
        tout.setInt32(r, 2, sumall & 0x00000000ffffffff);
        tout.setInt32(r, 3, sumall >> 32);
        // if(r<10)std::cout<<tout.getInt32(r, 0)<<"  "<<tout.getInt32(r,1)<<"
        // "<<std::dec<<tout.getInt32(r,2)<<std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q20GroupBy" << std::endl;
}
// void LineFilter table
void q20Join_t2_t3(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<Q20GroupKey, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        int32_t ps_suppkey = tin1.getInt32(i, 1);
        int32_t ps_availqty = tin1.getInt32(i, 2);
        Q20GroupKey k{p_partkey, ps_suppkey};
        ht1.insert(std::make_pair(k, ps_availqty));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_partkey = tin2.getInt32(i, 0);
        int32_t l_suppkey = tin2.getInt32(i, 1);
        Q20GroupKey k = {l_partkey, l_suppkey};
        auto it = ht1.find(k);
        if (it != ht1.end()) {
            int32_t ps_suppkey = it->first.l_suppkey;
            int32_t ps_availqty = it->second;
            // int64_t sum_t1 = tin2.getInt64(i,2);
            int32_t sum_l = tin2.getInt32(i, 2);
            int32_t sum_h = tin2.getInt32(i, 3);
            tout.setInt32(r, 0, ps_suppkey);
            tout.setInt32(r, 1, 10 * ps_availqty);
            tout.setInt32(r, 2, sum_l);
            tout.setInt32(r, 3, sum_h);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q20Join_t2_t3" << std::endl;
}

// filter table
void q20Join_s_t4(Table& tin1, Table& tin2, Table& tout) {
    struct q20KeyA {
        int32_t s_nationkey;
        int32_t s_rowid;
    };
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, q20KeyA> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_suppkey = tin1.getInt32(i, 0);
        int32_t s_nationkey = tin1.getInt32(i, 1);
        int32_t s_rowid = i; // tin1.getInt32(i,1);
        ht1.insert(std::make_pair(s_suppkey, q20KeyA{s_nationkey, s_rowid}));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_suppkey = tin2.getInt32(i, 0);
        int32_t ps_availqty = tin2.getInt32(i, 1);
        int32_t sum_l = tin2.getInt32(i, 2);
        int64_t sum_h = tin2.getInt32(i, 3);
        int64_t sum = (sum_h << 32) + sum_l;
        if (ps_availqty > sum) {
            auto it = ht1.find(ps_suppkey);
            if (it != ht1.end()) {
                int32_t s_nationkey = it->second.s_nationkey;
                int32_t s_rowid = it->second.s_rowid;
                tout.setInt32(r, 0, s_nationkey);
                tout.setInt32(r, 1, s_rowid);
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q20Join_s_t4" << std::endl;
}
void q20Join_t4_s(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t ps_suppkey = tin1.getInt32(i, 0);
        int32_t ps_availqty = tin1.getInt32(i, 1);
        int32_t sum_l = tin1.getInt32(i, 2);
        int64_t sum_h = tin1.getInt32(i, 3);
        int64_t sum = (sum_h << 32) + sum_l;
        if (ps_availqty > sum) {
            ht1.insert(std::make_pair(ps_suppkey, 0));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t s_suppkey = tin2.getInt32(i, 0);
        int32_t s_nationkey = tin2.getInt32(i, 1);
        int32_t s_rowid = i; // tin1.getInt32(i,1);
        auto it = ht1.find(s_suppkey);
        if (it != ht1.end()) {
            tout.setInt32(r, 0, s_nationkey);
            tout.setInt32(r, 1, s_rowid);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q20Join_t4_s" << std::endl;
}
// n_filter
// t6
void Filter_n(Table& tin, Table& tout) {
    int r = 0;
    int nrow = tin.getNumRow();
    for (int i = 0; i < nrow; i++) {
        int32_t n_nationkey = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 1);
        if (std::string(n_name.data()) == "CANADA") {
            tout.setInt32(r, 0, n_nationkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " Filter_n" << std::endl;
}
// filter table
void q20Join_t5_t6(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_nationkey = tin1.getInt32(i, 0);
        int32_t s_rowid = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(s_nationkey, s_rowid));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t n_nationkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(n_nationkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t s_rowid = it->second;
            tout.setInt32(r, 0, s_rowid);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q20Join_t5_t6" << std::endl;
}

void q20_Sort(Table& tin, Table& suppliertb, Table& tout) {
    struct Q20SortRow {
        // std::array<char,TPCH_READ_S_NAME_LEN+1>
        std::string s_address;
        std::string s_name;
    };

    struct {
        // operator <
        bool operator()(const Q20SortRow& a, const Q20SortRow& b) const { return a.s_name < b.s_name; }
    } Q20SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q20SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        int32_t rowid = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_S_ADDR_MAX + 1> s_address =
            suppliertb.getcharN<char, TPCH_READ_S_ADDR_MAX + 1>(rowid, 2);
        std::array<char, TPCH_READ_S_NAME_LEN + 1> s_name =
            suppliertb.getcharN<char, TPCH_READ_S_NAME_LEN + 1>(rowid, 3);
        Q20SortRow t = {std::string(s_address.data()), std::string(s_name.data())};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q20SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_S_ADDR_MAX + 1> s_address{};
        std::array<char, TPCH_READ_S_NAME_LEN + 1> s_name{};
        memcpy(s_address.data(), (it.s_address).data(), (it.s_address).length());
        memcpy(s_name.data(), (it.s_name).data(), (it.s_name).length());
        tout.setcharN<char, TPCH_READ_S_ADDR_MAX + 1>(r, 0, s_address);
        tout.setcharN<char, TPCH_READ_S_NAME_LEN + 1>(r, 1, s_name);
        if (r < 10) std::cout << s_name.data() << " " << s_address.data() << " " << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q20_Sort" << std::endl;
}
