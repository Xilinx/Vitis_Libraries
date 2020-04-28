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
// t1:5
void q5Join_r_n(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        std::array<char, TPCH_READ_REGION_LEN + 1> r_name = tin1.getcharN<char, TPCH_READ_REGION_LEN + 1>(i, 1);
        if (!strcmp("ASIA", r_name.data())) {
            // if(!strcmp("ASIA",r_name.data())){
            int32_t r_regionkey = tin1.getInt32(i, 0);
            ht1.insert(std::make_pair(r_regionkey, 0));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t n_regionkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(n_regionkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t n_nationkey = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, n_nationkey);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5Join_r_n" << std::endl;
}

// t2:1987
// s_nationkey,s_suppkey,s_rowid(s_acctbal,s_name,s_address,s_phone,s_comment)
void q5Join_t1_c(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t n_nationkey = tin1.getInt32(i, 0);
        ht1.insert(std::make_pair(n_nationkey, 0));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t c_nationkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(c_nationkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t c_custkey = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, c_custkey);
            tout.setInt32(r, 1, c_nationkey);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5Join_t1_c" << std::endl;
}

// t3:158960
void q5Join_t2_o(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t c_custkey = tin1.getInt32(i, 0);
        int32_t c_nationkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(c_custkey, c_nationkey));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t o_custkey = tin2.getInt32(i, 0);
        int32_t o_orderkey = tin2.getInt32(i, 1);
        int32_t o_orderdate = tin2.getInt32(i, 2);
        if (o_orderdate >= 19940101 && o_orderdate < 19950101) {
            auto its = ht1.equal_range(o_custkey);
            auto it = its.first;
            while (it != its.second) {
                tout.setInt32(r, 0, o_orderkey);
                tout.setInt32(r, 1, it->second);
                it++;
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5Join_t2_o" << std::endl;
}

void q5Join_t3_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t c_nationkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(o_orderkey, c_nationkey));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(l_orderkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t l_suppkey = tin2.getInt32(i, 1);
            int32_t l_extendedprice = tin2.getInt32(i, 2);
            int32_t l_discount = tin2.getInt32(i, 3);
            int32_t c_nationkey = it->second;
            int32_t e = l_extendedprice * (-l_discount + 100);
            tout.setInt32(r, 0, l_suppkey);
            tout.setInt32(r, 1, e);
            tout.setInt32(r, 2, c_nationkey);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5Join_t3_l" << std::endl;
}
struct KeyA {
    int32_t s_suppkey;
    int32_t s_nationkey;
    bool operator==(const KeyA& other) const {
        return (s_suppkey == other.s_suppkey) && (s_nationkey == other.s_nationkey);
    }
};
namespace std {
template <>
struct hash<KeyA> {
    std::size_t operator()(const KeyA& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<int>()(k.s_suppkey)) + (hash<int>()(k.s_nationkey));
    }
};
}
void q5Join_s_t4(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::cout << "s_t4:" << nrow1 << " " << nrow2 << std::endl;
    std::unordered_multimap<KeyA, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_suppkey = tin1.getInt32(i, 0);
        int32_t s_nationkey = tin1.getInt32(i, 1);
        KeyA keya{s_suppkey, s_nationkey};
        ht1.insert(std::make_pair(keya, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_suppkey = tin2.getInt32(i, 0);
        int32_t e = tin2.getInt32(i, 1);
        int32_t c_nationkey = tin2.getInt32(i, 2);
        auto its = ht1.equal_range(KeyA{l_suppkey, c_nationkey});
        auto it = its.first;
        while (it != its.second) {
            tout.setInt32(r, 0, e);
            tout.setInt32(r, 1, c_nationkey);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5Join_s_t4" << std::endl;
}
void q5Join_t5_n(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t e = tin1.getInt32(i, 0);
        int32_t s_nationkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(s_nationkey, e));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t n_nationkey = tin2.getInt32(i, 1);
        auto its = ht1.equal_range(n_nationkey);
        auto it = its.first;
        while (it != its.second) {
            std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin2.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 2);
            tout.setInt32(r, 0, it->second);
            tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 1, n_name);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5Join_t5_n" << std::endl;
}

struct q5GroupKey {
    std::string n_name;
    bool operator==(const q5GroupKey& other) const { return (n_name == other.n_name); }
};
namespace std {
template <>
struct hash<q5GroupKey> {
    std::size_t operator()(const q5GroupKey& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<string>()(k.n_name));
    }
};
}
void q5GroupBy(Table& tin, Table& tout) {
    std::unordered_map<q5GroupKey, int64_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t e = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 1);
        q5GroupKey k{std::string(n_name.data())};
        auto it = ht1.find(k);
        if (it != ht1.end()) {
            int64_t s = it->second + e;
            ht1[k] = s;
        } else {
            ht1.insert(std::make_pair(k, e));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name{};
        memcpy(n_name.data(), (it.first.n_name).data(), (it.first.n_name).length());
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 0, n_name);
        tout.setInt64(r, 1, it.second);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5GroupBy" << std::endl;
}

void q5Sort(Table& tin, Table& tout) {
    struct Q5SortRow {
        std::string n_name;
        int64_t e;
    };

    struct {
        // operator <
        bool operator()(const Q5SortRow& a, const Q5SortRow& b) const { return a.e > b.e; }
    } Q5SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q5SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 0);
        Q5SortRow t = {std::string(n_name.data()), tin.getInt64(i, 1)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q5SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name{};
        memcpy(n_name.data(), (it.n_name).data(), (it.n_name).length());
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 0, n_name);
        tout.setInt64(r, 1, it.e);
        if (r < 10)
            std::cout << std::dec << " " << tout.getcharN<char, TPCH_READ_NATION_LEN + 1>(r, 0).data() << " "
                      << tout.getInt64(r, 1) << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q5Sort" << std::endl;
}
