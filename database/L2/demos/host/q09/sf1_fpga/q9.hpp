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
void PartFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_NAME_LEN + 1> p_name = tin.getcharN<char, TPCH_READ_P_NAME_LEN + 1>(i, 1);
        //        if(std::regex_match(p_name.data(), std::regex("(.*)(green)(.*)"))){
        if (std::string(p_name.data()).find("green") != std::string::npos) {
            int32_t p_partkey = tin.getInt32(i, 0);
            tout.setInt32(r, 0, p_partkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after PartFilter" << std::endl;
}

// t2
void q9Join_t1_ps(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    std::cout << std::dec << ht1.size() << " " << std::endl;
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_partkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(ps_partkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t ps_suppkey = tin2.getInt32(i, 1);
            int32_t ps_supplycost = tin2.getInt32(i, 2);
            tout.setInt32(r, 0, ps_suppkey);
            tout.setInt32(r, 1, ps_partkey);
            tout.setInt32(r, 2, ps_supplycost);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q9Join_t1_ps" << std::endl;
}

// t3
void q9Join_s_t2(Table& tin1, Table& tin2, Table& tout) {
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
        int32_t ps_suppkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(ps_suppkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t ps_partkey = tin2.getInt32(i, 1);
            int32_t ps_supplycost = tin2.getInt32(i, 2);
            int32_t s_nationkey = it->second;
            tout.setInt32(r, 0, ps_suppkey);
            tout.setInt32(r, 1, ps_partkey);
            tout.setInt32(r, 2, s_nationkey);
            tout.setInt32(r, 3, ps_supplycost);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q9Join_s_t2" << std::endl;
}

// t4
struct q9hashkey {
    int32_t s_suppkey;
    int32_t ps_partkey;
    bool operator==(const q9hashkey& other) const {
        return (s_suppkey == other.s_suppkey) && (ps_partkey == other.ps_partkey);
    }
};
namespace std {
template <>
struct hash<q9hashkey> {
    std::size_t operator()(const q9hashkey& k) const {
        using std::size_t;
        using std::hash;
        return (hash<int>()(k.s_suppkey)) + (hash<int>()(k.ps_partkey));
    }
};
}
void q9Join_t3_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();

    struct q9hashvalue {
        int32_t s_nationkey;
        int32_t ps_supplycost;
    };

    std::unordered_multimap<q9hashkey, q9hashvalue> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_suppkey = tin1.getInt32(i, 0);
        int32_t ps_partkey = tin1.getInt32(i, 1);
        int32_t s_nationkey = tin1.getInt32(i, 2);
        int32_t ps_supplycost = tin1.getInt32(i, 3);
        ht1.insert(std::make_pair(q9hashkey{s_suppkey, ps_partkey}, q9hashvalue{s_nationkey, ps_supplycost}));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_suppkey = tin2.getInt32(i, 0);
        int32_t l_partkey = tin2.getInt32(i, 1);
        auto its = ht1.equal_range(q9hashkey{l_suppkey, l_partkey});
        auto it = its.first;
        while (it != its.second) {
            int32_t ps_supplycost = it->second.ps_supplycost;
            int32_t s_nationkey = it->second.s_nationkey;
            int32_t l_orderkey = tin2.getInt32(i, 2);
            int32_t l_extendedprice = tin2.getInt32(i, 3);
            int32_t l_discount = tin2.getInt32(i, 4);
            int32_t l_quantity = tin2.getInt32(i, 5);
            int32_t e = l_extendedprice * (100 - l_discount) - 100 * ps_supplycost * l_quantity;
            // if(r<10) std::cout<<s_nationkey<<" "<<l_orderkey<<std::endl;
            // if(r<10) std::cout<<l_extendedprice<<" "<<l_discount<<" "<<l_quantity<<" "<<ps_supplycost<<"
            // "<<e<<std::endl;

            tout.setInt32(r, 0, l_orderkey);
            tout.setInt32(r, 1, s_nationkey);
            tout.setInt32(r, 2, e);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q9Join_t3_l" << std::endl;
}
// t5
void q9Join_o_t4(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t o_orderdate = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(o_orderkey, o_orderdate));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(l_orderkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t s_nationkey = tin2.getInt32(i, 1);
            int32_t eval0 = tin2.getInt32(i, 2);
            int32_t o_orderdate = it->second;
            tout.setInt32(r, 0, s_nationkey);
            tout.setInt32(r, 1, o_orderdate);
            tout.setInt32(r, 2, eval0);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q9Join_o_t4" << std::endl;
}
// std::array<char,TPCH_READ_NATION_LEN+1>
void q9Join_n_t5(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t n_nationkey = tin1.getInt32(i, 0);
        int32_t n_rowid = tin1.getInt32(i, 2);
        // std::array<char,TPCH_READ_NATION_LEN+1> n_name = tin1.getcharN<char,TPCH_READ_NATION_LEN+1>(i,0);
        ht1.insert(std::make_pair(n_nationkey, n_rowid));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t s_nationkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(s_nationkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t o_orderdate = tin2.getInt32(i, 1);
            int32_t eval0 = tin2.getInt32(i, 2);
            int32_t n_rowid = it->second;
            //  std::array<char,TPCH_READ_NATION_LEN+1> n_name = it->second;
            //  tout.setcharN<char,TPCH_READ_NATION_LEN+1>(i,0,n_name);
            tout.setInt32(r, 0, n_rowid);
            tout.setInt32(r, 1, o_orderdate);
            tout.setInt32(r, 2, eval0);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q9Join_n_t5" << std::endl;
}

// t8
struct q9GroupByKey {
    std::string n_name;
    int32_t o_orderdate_year;
    bool operator==(const q9GroupByKey& other) const {
        return (n_name == other.n_name) && (o_orderdate_year == other.o_orderdate_year);
    }
};
namespace std {
template <>
struct hash<q9GroupByKey> {
    std::size_t operator()(const q9GroupByKey& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<string>()(k.n_name)) + (hash<int>()(k.o_orderdate_year));
    }
};
}
void q9GroupBy(Table& tin, Table& origin, Table& tout) {
    std::unordered_map<q9GroupByKey, int64_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t n_rowid = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = origin.getcharN<char, TPCH_READ_NATION_LEN + 1>(n_rowid, 1);
        //       if(i<30) std::cout<<n_name.data()<<" "<<std::endl;
        int32_t o_orderdate = tin.getInt32(i, 1) / 10000; // get the year
        int32_t e = tin.getInt32(i, 2);
        q9GroupByKey k{std::string(n_name.data()), o_orderdate};
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
        std::string n_name = it.first.n_name;
        std::array<char, TPCH_READ_NATION_LEN + 1> name{};
        memcpy(name.data(), n_name.data(), n_name.length());
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 0, name);
        tout.setInt32(r, 1, it.first.o_orderdate_year);
        tout.setInt64(r, 2, it.second);
        //        if(n_name=="CHINA") std::cout<<n_name<<" "<<it.second<<std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q9GroupBy" << std::endl;
}

void q9Sort(Table& tin, Table& tout) {
    struct Q9SortRow {
        std::string n_name;
        int32_t o_orderdate_year;
        int64_t e;
    };

    struct {
        // operator <
        bool operator()(const Q9SortRow& a, const Q9SortRow& b) const {
            return a.n_name < b.n_name || (a.n_name == b.n_name && a.o_orderdate_year > b.o_orderdate_year);
        }
    } Q9SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q9SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 0);
        Q9SortRow t = {std::string(n_name.data()), tin.getInt32(i, 1), tin.getInt64(i, 2)};
        // if(i<30) std::cout<<n_name.data()<<" "<<std::endl;
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q9SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::string n_name = it.n_name;
        std::array<char, TPCH_READ_NATION_LEN + 1> name{};
        memcpy(name.data(), n_name.data(), n_name.length());
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 0, name);
        tout.setInt32(r, 1, it.o_orderdate_year);
        tout.setInt64(r, 2, it.e);
        if (r < 10) std::cout << std::dec << name.data() << " " << it.o_orderdate_year << " " << it.e << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q9Sort" << std::endl;
}
