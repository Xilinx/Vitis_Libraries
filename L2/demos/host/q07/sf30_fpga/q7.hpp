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
// t1 //t6
void NationFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 1);
        if (!strcmp("FRANCE", n_name.data()) || !strcmp("GERMANY", n_name.data())) {
            int32_t n_nationkey = tin.getInt32(i, 0);
            int32_t n_rowid = i;
            tout.setInt32(r, 0, n_nationkey);
            tout.setInt32(r, 1, n_rowid);
            // tout.setcharN<char,TPCH_READ_NATION_LEN + 1>(r,1,n_name);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In NationFilter" << std::endl;
}

// t2  nation: n_nationkey,n_name,n_rowid
void q7Join_t1_s(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), tin1.getInt32(i, 1)));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t s_nationkey = tin2.getInt32(i, 0);
        auto it = ht1.find(s_nationkey);
        if (it != ht1.end()) {
            int32_t s_suppkey = tin2.getInt32(i, 1);
            int32_t n_rowid = it->second;
            tout.setInt32(r, 0, s_nationkey);
            tout.setInt32(r, 1, n_rowid);
            tout.setInt32(r, 2, s_suppkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q7Join_t1_s" << std::endl;
}

// t3
void q7Join_t2_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    struct Key {
        int32_t n_nationkey;
        int32_t n_rowid;
    };

    std::unordered_map<int32_t, Key> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t n_nationkey = tin1.getInt32(i, 0);
        int32_t n_rowid = tin1.getInt32(i, 1);
        int32_t s_suppkey = tin1.getInt32(i, 2);
        ht1.insert(std::make_pair(s_suppkey, Key{n_nationkey, n_rowid}));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t l_suppkey = tin2.getInt32(i, 1);
        int32_t l_shipdate = tin2.getInt32(i, 2);
        int32_t l_extendedprice = tin2.getInt32(i, 3);
        int32_t l_discount = tin2.getInt32(i, 4);
        if (l_shipdate >= 19950101 && l_shipdate <= 19961231) {
            auto it = ht1.find(l_suppkey);
            if (it != ht1.end()) {
                int32_t n_nationkey = it->second.n_nationkey;
                int32_t n_rowid = it->second.n_rowid;
                int32_t e = l_extendedprice * (100 - l_discount);
                tout.setInt32(r, 0, n_nationkey);
                tout.setInt32(r, 1, n_rowid);
                tout.setInt32(r, 2, l_orderkey);
                tout.setInt32(r, 3, l_shipdate);
                tout.setInt32(r, 4, e);
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q7Join_t2_l" << std::endl;
}

// t4
void q7Join_o_t3(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t o_custkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(o_orderkey, o_custkey));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 2);
        auto it = ht1.find(l_orderkey);
        if (it != ht1.end()) {
            int32_t n_nationkey = tin2.getInt32(i, 0);
            int32_t n_rowid = tin2.getInt32(i, 1);
            int32_t o_custkey = it->second;
            int32_t l_shipdate = tin2.getInt32(i, 3);
            int32_t e = tin2.getInt32(i, 4);
            tout.setInt32(r, 0, n_nationkey);
            tout.setInt32(r, 1, n_rowid);
            tout.setInt32(r, 2, o_custkey);
            tout.setInt32(r, 3, l_shipdate);
            tout.setInt32(r, 4, e);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q7Join_o_t3" << std::endl;
}
// t5
void q7Join_c_t4(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t c_custkey = tin1.getInt32(i, 0);
        int32_t c_nationkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(c_custkey, c_nationkey));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t o_custkey = tin2.getInt32(i, 2);
        auto it = ht1.find(o_custkey);
        if (it != ht1.end()) {
            int32_t n_nationkey = tin2.getInt32(i, 0);
            int32_t n_rowid = tin2.getInt32(i, 1);
            int32_t c_nationkey = it->second;
            int32_t l_shipdate = tin2.getInt32(i, 3);
            int32_t e = tin2.getInt32(i, 4);
            tout.setInt32(r, 0, n_nationkey);
            tout.setInt32(r, 1, n_rowid);
            tout.setInt32(r, 2, c_nationkey);
            tout.setInt32(r, 3, l_shipdate);
            tout.setInt32(r, 4, e);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q7Join_o_t3" << std::endl;
}

// t6 is t1
void q7Join_t6_t5(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t n_nationkey = tin1.getInt32(i, 0);
        int32_t n_rowid = tin1.getInt32(i, 1);
        ;
        ht1.insert(std::make_pair(n_nationkey, n_rowid));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t n_nationkey = tin2.getInt32(i, 0);
        int32_t n_rowid_t5 = tin2.getInt32(i, 1);
        int32_t c_nationkey = tin2.getInt32(i, 2);
        int32_t l_shipdate = tin2.getInt32(i, 3);
        int32_t e = tin2.getInt32(i, 4);
        if (n_nationkey != c_nationkey) {
            auto it = ht1.find(c_nationkey);
            if (it != ht1.end()) {
                int32_t n_rowid_t6 = it->second;
                tout.setInt32(r, 0, n_rowid_t5);
                tout.setInt32(r, 1, n_rowid_t6);
                tout.setInt32(r, 2, l_shipdate);
                tout.setInt32(r, 3, e);
                r++;
            }
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q7Join_t6_t5" << std::endl;
}

// t8
struct q7GroupBy {
    std::string supp_nation; // int32_t n_rowid_t5;
    std::string cust_nation; // int32_t n_rowid_t6;
    int32_t l_shipdate;
    bool operator==(const q7GroupBy& other) const {
        return (supp_nation == other.supp_nation) && (cust_nation == other.cust_nation) &&
               (l_shipdate == other.l_shipdate);
    }
};
namespace std {
template <>
struct hash<q7GroupBy> {
    std::size_t operator()(const q7GroupBy& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<string>()(k.supp_nation)) + (hash<string>()(k.cust_nation)) + (hash<int>()(k.l_shipdate));
    }
};
}
void q7Group(Table& tin, Table& origin, Table& tout) {
    std::unordered_map<q7GroupBy, int64_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t n_rowid_t5 = tin.getInt32(i, 0);
        int32_t n_rowid_t6 = tin.getInt32(i, 1);
        int32_t l_shipdate = tin.getInt32(i, 2) / 10000;
        int32_t e = tin.getInt32(i, 3);
        std::array<char, TPCH_READ_NATION_LEN + 1> supp_nation =
            origin.getcharN<char, TPCH_READ_NATION_LEN + 1>(n_rowid_t5, 1);
        std::array<char, TPCH_READ_NATION_LEN + 1> cust_nation =
            origin.getcharN<char, TPCH_READ_NATION_LEN + 1>(n_rowid_t6, 1);
        q7GroupBy k{std::string(supp_nation.data()), std::string(cust_nation.data()), l_shipdate};
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
        std::array<char, TPCH_READ_NATION_LEN + 1> supp_nation{};
        std::array<char, TPCH_READ_NATION_LEN + 1> cust_nation{};
        memcpy(supp_nation.data(), it.first.supp_nation.data(), it.first.supp_nation.length());
        memcpy(cust_nation.data(), it.first.cust_nation.data(), it.first.cust_nation.length());
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 0, supp_nation);
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 1, cust_nation);
        tout.setInt32(r, 2, it.first.l_shipdate);
        tout.setInt64(r, 3, it.second);
        //        if(r<10) std::cout<<it.second<<std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q7GroupBy" << std::endl;
}
void q7Sort(Table& tin, Table& tout) {
    struct Q7SortRow {
        std::string supp_nation; // int32_t n_rowid_t5;
        std::string cust_nation; // int32_t n_rowid_t6;
        int32_t l_shipdate;
        int64_t e;
    };

    struct {
        // operator <
        bool operator()(const Q7SortRow& a, const Q7SortRow& b) const {
            return (a.supp_nation < b.supp_nation) ||
                   (a.supp_nation == b.supp_nation && a.cust_nation < b.cust_nation) ||
                   (a.supp_nation == b.supp_nation && a.cust_nation == b.cust_nation && a.l_shipdate < b.l_shipdate);
        }
    } Q7SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q7SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_NATION_LEN + 1> supp_nation = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 0);
        std::array<char, TPCH_READ_NATION_LEN + 1> cust_nation = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 1);
        Q7SortRow t = {std::string(supp_nation.data()), std::string(cust_nation.data()), tin.getInt32(i, 2),
                       tin.getInt64(i, 3)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q7SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_NATION_LEN + 1> supp_nation{};
        std::array<char, TPCH_READ_NATION_LEN + 1> cust_nation{};
        memcpy(supp_nation.data(), it.supp_nation.data(), it.supp_nation.length());
        memcpy(cust_nation.data(), it.cust_nation.data(), it.cust_nation.length());
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 0, supp_nation);
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 1, cust_nation);
        tout.setInt64(r, 2, it.l_shipdate);
        tout.setInt64(r, 3, it.e);
        if (r < 10)
            std::cout << std::dec << cust_nation.data() << " " << supp_nation.data() << " " << it.l_shipdate << " "
                      << it.e << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q7Sort" << std::endl;
}
