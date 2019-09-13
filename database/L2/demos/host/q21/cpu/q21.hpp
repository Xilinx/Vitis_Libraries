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
#include <set>
// t0
void NationFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        int32_t n_nationkey = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 1);
        if (std::string(n_name.data()) == "SAUDI ARABIA") {
            tout.setInt32(r, 0, n_nationkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after NationFilter" << std::endl;
}
void q21Join_n_s(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t n_nationkey = tin1.getInt32(i, 0);
        ht1.insert(std::make_pair(n_nationkey, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t s_suppkey = tin2.getInt32(i, 0);
        int32_t s_nationkey = tin2.getInt32(i, 1);
        auto it = ht1.find(s_nationkey);
        if (it != ht1.end()) {
            tout.setInt32(r, 0, s_suppkey);
            tout.setInt32(r, 1, s_nationkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21Join_n_s" << std::endl;
}
// t1
void q21Join_t0_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_suppkey = tin1.getInt32(i, 0);
        int32_t s_nationkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(s_suppkey, s_nationkey));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t l_suppkey = tin2.getInt32(i, 1);
        int32_t l_receiptdate = tin2.getInt32(i, 2);
        int32_t l_commitdate = tin2.getInt32(i, 3);
        if (l_receiptdate > l_commitdate) {
            auto it = ht1.find(l_suppkey);
            if (it != ht1.end()) {
                tout.setInt32(r, 0, l_orderkey);
                tout.setInt32(r, 1, it->second);
                tout.setInt32(r, 2, l_suppkey);
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21Join_t0_l" << std::endl;
}
// t2
void q21Join_o_t1(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t o_orderstatus = tin1.getInt32(i, 1);
        if (o_orderstatus == 'F') {
            ht1.insert(std::make_pair(o_orderkey, 0));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t s_nationkey = tin2.getInt32(i, 1);
        int32_t l_suppkey = tin2.getInt32(i, 2);
        auto it = ht1.find(l_orderkey);
        if (it != ht1.end()) {
            tout.setInt32(r, 0, l_orderkey);
            tout.setInt32(r, 1, s_nationkey);
            tout.setInt32(r, 2, l_suppkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21Join_o_t1" << std::endl;
}
struct Q21AntiKey {
    int32_t l_orderkey;
    int32_t l_suppkey;
    // std::array

    bool operator==(const Q21AntiKey& other) const {
        return (l_orderkey == other.l_orderkey) && (l_suppkey != other.l_suppkey);
    }
};
namespace std {

template <>
struct hash<Q21AntiKey> {
    std::size_t operator()(const Q21AntiKey& k) const {
        using std::size_t;
        using std::hash;
        return (hash<int>()(k.l_orderkey));
    }
};
}
// t3
void q21SemiJoin_l_t2(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<Q21AntiKey, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t l_orderkey = tin1.getInt32(i, 0);
        int32_t l_suppkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(Q21AntiKey{l_orderkey, l_suppkey}, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t s_nationkey = tin2.getInt32(i, 1);
        int32_t s_suppkey = tin2.getInt32(i, 2);
        Q21AntiKey k = {l_orderkey, s_suppkey};
        auto it = ht1.find(k);
        if (it != ht1.end()) {
            //            if(r<10) std::cout<<l_orderkey<<" "<<it->first.l_orderkey<<std::endl;
            //            if(r<10) std::cout<<s_suppkey<<" "<<it->first.l_suppkey<<std::endl;
            tout.setInt32(r, 0, l_orderkey);
            tout.setInt32(r, 1, s_nationkey);
            tout.setInt32(r, 2, s_suppkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21SemiJoin_l_t2" << std::endl;
}
// t4
void q21SemiJoin_l_t3(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<Q21AntiKey, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t l_orderkey = tin1.getInt32(i, 0);
        int32_t l_suppkey = tin1.getInt32(i, 1);
        int32_t l_receiptdate = tin1.getInt32(i, 2);
        int32_t l_commitdate = tin1.getInt32(i, 3);
        if (l_receiptdate > l_commitdate) {
            ht1.insert(std::make_pair(Q21AntiKey{l_orderkey, l_suppkey}, 0));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t s_nationkey = tin2.getInt32(i, 1);
        int32_t s_suppkey = tin2.getInt32(i, 2);
        Q21AntiKey k = {l_orderkey, s_suppkey};
        auto it = ht1.find(k);
        if (it != ht1.end()) {
            tout.setInt32(r, 0, l_orderkey);
            tout.setInt32(r, 1, s_nationkey);
            tout.setInt32(r, 2, s_suppkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21SemiJoin_l_t3" << std::endl;
}
struct KeyA {
    int32_t l_orderkey;
    int32_t l_suppkey;
    bool operator==(const KeyA& other) const {
        return (l_orderkey == other.l_orderkey) && (l_suppkey == other.l_suppkey);
    }
};
namespace std {

template <>
struct hash<KeyA> {
    std::size_t operator()(const KeyA& k) const {
        using std::size_t;
        using std::hash;
        return (hash<int>()(k.l_orderkey)) + (hash<int>()(k.l_suppkey));
    }
};
}
void q21AntiJoin_t4_t3(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<KeyA, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t l_orderkey = tin1.getInt32(i, 0);
        int32_t l_suppkey = tin1.getInt32(i, 2);
        ht1.insert(std::make_pair(KeyA{l_orderkey, l_suppkey}, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t s_nationkey = tin2.getInt32(i, 1);
        int32_t s_suppkey = tin2.getInt32(i, 2);
        KeyA k = {l_orderkey, s_suppkey};
        auto it = ht1.find(k);
        if (it == ht1.end()) {
            tout.setInt32(r, 0, s_suppkey);
            tout.setInt32(r, 1, s_nationkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21AntiJoin_t4_t3" << std::endl;
}
// cancle in this file
void q21AntiJoin_l_t3(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<Q21AntiKey, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t l_orderkey = tin1.getInt32(i, 0);
        int32_t l_suppkey = tin1.getInt32(i, 1);
        int32_t l_receiptdate = tin1.getInt32(i, 2);
        int32_t l_commitdate = tin1.getInt32(i, 3);
        if (l_receiptdate > l_commitdate) {
            ht1.insert(std::make_pair(Q21AntiKey{l_orderkey, l_suppkey}, 0));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t s_nationkey = tin2.getInt32(i, 1);
        int32_t s_suppkey = tin2.getInt32(i, 2);
        Q21AntiKey k = {l_orderkey, s_suppkey};
        auto it = ht1.find(k);
        if (it == ht1.end()) {
            tout.setInt32(r, 0, s_suppkey);
            tout.setInt32(r, 1, s_nationkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21AntiJoin_l_t3" << std::endl;
}
void q21Group_t4(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow; i++) {
        int32_t s_suppkey = tin.getInt32(i, 0);
        auto it = ht1.find(s_suppkey);
        if (it != ht1.end()) {
            int32_t t = it->second + 1;
            ht1[s_suppkey] = t;
        } else {
            ht1.insert(std::make_pair(s_suppkey, 1));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        tout.setInt32(r, 0, it.first);
        tout.setInt32(r, 1, it.second);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21Group_t4" << std::endl;
}
// t4
// t7

void q21Sort(Table& tin, Table& tout) {
    struct Q21SortRow {
        int32_t s_suppkey;
        int32_t cnt;
    };

    struct {
        // operator <
        bool operator()(const Q21SortRow& a, const Q21SortRow& b) const {
            return a.cnt > b.cnt || (a.cnt == b.cnt && a.s_suppkey < b.s_suppkey);
        }
    } Q21SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q21SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        Q21SortRow t = {tin.getInt32(i, 0), tin.getInt32(i, 1)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q21SortLess);

    int r = 0;
    for (auto& it : rows) {
        ;
        tout.setInt32(r, 0, it.s_suppkey);
        tout.setInt32(r, 1, it.cnt);
        if (r < 10) std::cout << std::dec << tout.getInt32(r, 0) << " " << tout.getInt32(r, 1) << " " << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q21Sort" << std::endl;
}
