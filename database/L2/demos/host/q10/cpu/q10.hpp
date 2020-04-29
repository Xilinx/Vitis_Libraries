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
#include <unordered_map>
// order and lineitem
void q10Join_O_L(Table& tin1, Table& tin2, Table& tout) {
    std::unordered_map<int32_t, int32_t> ht1;
    int nrow1 = tin1.getNumRow();
    for (int i = 0; i < nrow1; ++i) {
        int o_orderdate = tin1.getInt32(i, 0);
        if (19931001 <= o_orderdate && o_orderdate < 19940101) {
            int o_orderkey = tin1.getInt32(i, 1);
            int o_custkey = tin1.getInt32(i, 2);
            ht1.insert(std::make_pair(o_orderkey, o_custkey));
        }
    }
    struct lineitem {
        int32_t l_orderkey;
        int32_t l_extendedprice;
        int32_t l_discount;
    };
    std::vector<lineitem> ht2;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; ++i) {
        int l_returnflag = tin2.getInt32(i, 0);
        if (l_returnflag == 'R') { // 82
            // l_orderkey,l_extendedprice,l_discount
            int l_orderkey = tin2.getInt32(i, 1);
            int l_extendedprice = tin2.getInt32(i, 2);
            int l_discount = tin2.getInt32(i, 3);
            ht2.push_back(lineitem{l_orderkey, l_extendedprice, l_discount});
        }
    }
    int r = 0;
    for (size_t i = 0; i < ht2.size(); i++) {
        int32_t l_orderkey = ht2[i].l_orderkey;
        auto it1 = ht1.find(l_orderkey);
        if (it1 != ht1.end()) {
            int o_custkey = it1->second;
            int l_extendedprice = ht2[i].l_extendedprice;
            int l_discount = ht2[i].l_discount;
            int32_t revenue = (-l_discount + 100) * l_extendedprice;
            tout.setInt32(r, 0, o_custkey);
            tout.setInt32(r, 1, revenue);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q10Join_O_L" << std::endl;
}
void q10Join_C_O1(Table& tin1, Table& tin2, Table& tout) {
    struct KeyC {
        int32_t c_nationkey;
        int32_t c_rowid;
    };

    std::unordered_map<int32_t, KeyC> ht1;
    int nrow1 = tin1.getNumRow();
    for (int i = 0; i < nrow1; ++i) {
        int c_custkey = tin1.getInt32(i, 0);
        int c_nationkey = tin1.getInt32(i, 1);
        int c_rowid = tin1.getInt32(i, 2);
        ht1.insert(std::make_pair(c_custkey, KeyC{c_nationkey, c_rowid}));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int o_custkey = tin2.getInt32(i, 0);
        auto it = ht1.find(o_custkey);
        if (it != ht1.end()) {
            int c_nationkey = it->second.c_nationkey;
            int c_rowid = it->second.c_rowid;
            int revenue = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, c_nationkey);
            tout.setInt32(r, 1, o_custkey);
            tout.setInt32(r, 2, c_rowid);
            tout.setInt32(r, 3, revenue);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q10Join_C_O1" << std::endl;
}
void q10Join_N_O2(Table& tin1, Table& tin2, Table& tout) {
    std::unordered_map<int32_t, int32_t> ht1;
    int nrow1 = tin1.getNumRow();
    for (int i = 0; i < nrow1; ++i) {
        int n_nationkey = tin1.getInt32(i, 0);
        int n_rowid = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(n_nationkey, n_rowid));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int c_nationkey = tin2.getInt32(i, 0);
        auto it = ht1.find(c_nationkey);
        if (it != ht1.end()) {
            int o_custkey = tin2.getInt32(i, 1);
            int c_rowid = tin2.getInt32(i, 2);
            int n_rowid = it->second;
            int revenue = tin2.getInt32(i, 3);
            tout.setInt32(r, 0, o_custkey);
            tout.setInt32(r, 1, c_nationkey);
            tout.setInt32(r, 2, n_rowid);
            tout.setInt32(r, 3, c_rowid);
            tout.setInt32(r, 4, revenue);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q10Join_N_O2" << std::endl;
}
struct Q10GroupKey {
    int32_t c_custkey;
    int32_t n_nationkey;
    int32_t n_rowid;
    int32_t c_rowid;
    // std::array

    bool operator==(const Q10GroupKey& other) const { return (c_custkey == other.c_custkey); }
};
namespace std {

template <>
struct hash<Q10GroupKey> {
    std::size_t operator()(const Q10GroupKey& k) const {
        using std::size_t;
        using std::hash;
        return (hash<int>()(k.c_custkey));
    }
};
}

void q10GroupBy(Table& tin, Table& tout) {
    std::unordered_map<Q10GroupKey, int64_t> m;
    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int32_t c_custkey = tin.getInt32(i, 0);   // index much patMatch YAML
        int32_t n_nationkey = tin.getInt32(i, 1); // index much patMatch YAML
        int32_t n_rowid = tin.getInt32(i, 2);
        int32_t c_rowid = tin.getInt32(i, 3);
        int32_t revenue = tin.getInt32(i, 4);

        Q10GroupKey k{c_custkey, n_nationkey, n_rowid, c_rowid};
        auto it = m.find(k);
        if (it != m.end()) {
            int64_t s = it->second + revenue;
            m[k] = s; // update
        } else {
            m.insert(std::make_pair(k, revenue));
        }
    }
    int r = 0;
    for (auto& it : m) {
        tout.setInt32(r, 0, it.first.c_custkey);
        tout.setInt32(r, 1, it.first.n_nationkey);
        tout.setInt32(r, 2, it.first.n_rowid);
        tout.setInt32(r, 3, it.first.c_rowid);
        tout.setInt64(r, 4, it.second);
        ++r;
    }
    tout.setNumRow(r);
}

//-------------------------------------------------------

// tin:  l_orderkey, revenue, o_orderdate, o_shipdate
// tout:  l_orderkey, revenue, o_orderdate, o_shipdate
// cfg:  o_
void q10Sort(Table& tin, Table& tout) {
    struct Q10SortRow {
        int32_t c_custkey;
        int32_t c_nationkey;
        int32_t n_rowid;
        int32_t c_rowid;
        int64_t revenue;
    };

    struct {
        // operator <
        bool operator()(const Q10SortRow& a, const Q10SortRow& b) const {
            return a.revenue > b.revenue || (a.revenue == b.revenue && a.c_custkey < b.c_custkey);
        }
    } Q10SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q10SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        Q10SortRow t = {tin.getInt32(i, 0), tin.getInt32(i, 1), tin.getInt32(i, 2), tin.getInt32(i, 3),
                        tin.getInt64(i, 4)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q10SortLess);

    int r = 0;
    for (auto& it : rows) {
        tout.setInt32(r, 0, it.c_custkey);
        tout.setInt32(r, 1, it.c_nationkey);
        tout.setInt32(r, 2, it.n_rowid);
        tout.setInt32(r, 3, it.c_rowid);
        tout.setInt64(r, 4, it.revenue);
        ++r;
    }
    tout.setNumRow(r);
}
void q10Print(Table& tin) {
    unsigned nrow = tin.getNumRow();
    printf("after sort:%d\n", nrow);
    int r = nrow > 32 ? 32 : nrow;
    std::cout << "Query result:" << nrow << std::endl;
    for (int i = 0; i < r; ++i) {
        int32_t key = tin.getInt32(i, 0); // index much patMatch YAML
        int32_t n_nationkey = tin.getInt32(i, 1);
        int32_t n_rowid = tin.getInt32(i, 2);
        int32_t c_rowid = tin.getInt32(i, 3);
        int64_t eval0 = tin.getInt64(i, 4);
        printf("key:%d,n_nationkey:%d,n_rowid:%d,,c_rowid:%d, eval0:%ld\n", key, n_nationkey, n_rowid, c_rowid, eval0);
    }
}

void gathertable(Table& tin, Table& origin1, Table& origin2, Table& tout) {
    int nrow = tin.getNumRow();
    for (int i = 0; i < nrow; i++) {
        int32_t key = tin.getInt32(i, 0); // index much patMatch YAML
        int32_t n_nationkey = tin.getInt32(i, 1);
        int32_t c_rowid = tin.getInt32(i, 3);
        int64_t eval0 = tin.getInt64(i, 4);
        // std::cout<<std::dec<<c_rowid<<std::endl;
        std::array<char, TPCH_READ_C_NAME_LEN + 1> cname = origin1.getcharN<char, TPCH_READ_C_NAME_LEN + 1>(c_rowid, 3);
        int32_t acctbal = origin1.getInt32(c_rowid, 4);
        //      std::array<char,TPCH_READ_C_ADDR_MAX + 1> addr = origin1.getcharN<char,TPCH_READ_C_ADDR_MAX +
        //      1>(c_rowid,5);
        //       std::array<char,TPCH_READ_PHONE_LEN + 1> phone = origin1.getcharN<char,TPCH_READ_PHONE_LEN +
        //       1>(c_rowid,6);
        //        std::array<char,TPCH_READ_NATION_LEN+1> nname = origin2.getcharN<char,TPCH_READ_NATION_LEN +
        //        1>(n_rowid,2);
        tout.setInt32(i, 0, key);
        tout.setInt64(i, 1, eval0);
        tout.setInt32(i, 2, n_nationkey);
        tout.setInt32(i, 3, acctbal);
        tout.setcharN<char, TPCH_READ_C_NAME_LEN + 1>(i, 4, cname);
        //      tout.setcharN<char,TPCH_READ_NATION_LEN + 1>(i,5,nname);
        //       tout.setcharN<char,TPCH_READ_C_ADDR_MAX + 1>(i,6,addr);
        //        tout.setcharN<char,TPCH_READ_PHONE_LEN + 1>(i,7,phone);
    }
}
void q10PrintAll(Table& tin) {
    unsigned nrow = tin.getNumRow();
    printf("after sort:%d\n", nrow);
    int r = nrow > 32 ? 32 : nrow;
    std::cout << "Query result:" << nrow << std::endl;
    for (int i = 0; i < r; ++i) {
        int32_t key = tin.getInt32(i, 0); // index much patMatch YAML
        int64_t eval0 = tin.getInt64(i, 1);
        int32_t n_nationkey = tin.getInt32(i, 2);
        int32_t acctbal = tin.getInt32(i, 3);
        std::array<char, TPCH_READ_C_NAME_LEN + 1> cname = tin.getcharN<char, TPCH_READ_C_NAME_LEN + 1>(i, 4);
        // std::array<char,TPCH_READ_NATION_LEN+1> nname = tin.getcharN<char,TPCH_READ_NATION_LEN + 1>(i,5);
        //     std::array<char,TPCH_READ_C_ADDR_MAX + 1> addr = tin.getcharN<char,TPCH_READ_C_ADDR_MAX + 1>(i,6);
        //   std::array<char,TPCH_READ_PHONE_LEN + 1> phone = tin.getcharN<char,TPCH_READ_PHONE_LEN + 1>(i,7);

        printf("key:%d, eval0:%ld, n_nationkey:%d, acctbal:%d, cname:%s\n ", key, eval0, n_nationkey, acctbal,
               cname.data());
    }
}
