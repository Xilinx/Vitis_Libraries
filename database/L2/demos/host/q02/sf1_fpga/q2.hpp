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
void q2Join_r_n(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        std::array<char, TPCH_READ_REGION_LEN + 1> r_name = tin1.getcharN<char, TPCH_READ_REGION_LEN + 1>(i, 1);
        if (!strcmp("EUROPE", r_name.data())) {
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
            int32_t n_rowid = i;
            // std::array<char,TPCH_READ_NATION_LEN+1>n_name = tin2.getcharN<char,TPCH_READ_NATION_LEN+1>(i,2);
            tout.setInt32(r, 0, n_nationkey);
            tout.setInt32(r, 1, n_rowid);
            //  tout.setcharN<char,TPCH_READ_NATION_LEN+1>(r,1,n_name.data());
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2Join_r_n" << std::endl;
}

// t2:1987
// s_nationkey,s_suppkey,s_rowid(s_acctbal,s_name,s_address,s_phone,s_comment)
void q2Join_t1_s(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t n_nationkey = tin1.getInt32(i, 0);
        int32_t n_rowid = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(n_nationkey, n_rowid));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t s_nationkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(s_nationkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t s_suppkey = tin2.getInt32(i, 1);
            int32_t s_rowid = tin2.getInt32(i, 7);
            int32_t n_rowid = it->second;
            tout.setInt32(r, 0, s_suppkey);
            tout.setInt32(r, 1, s_rowid);
            tout.setInt32(r, 2, n_rowid);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2Join_t1_s" << std::endl;
}

// t3:158960
void q2Join_t2_p(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    struct KeyA {
        int32_t s_rowid;
        int32_t n_rowid;
    };
    std::unordered_multimap<int32_t, KeyA> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_suppkey = tin1.getInt32(i, 0);
        int32_t s_rowid = tin1.getInt32(i, 1);
        int32_t n_rowid = tin1.getInt32(i, 2);
        ht1.insert(std::make_pair(s_suppkey, KeyA{s_rowid, n_rowid}));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_suppkey = tin2.getInt32(i, 1);
        auto its = ht1.equal_range(ps_suppkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t ps_partkey = tin2.getInt32(i, 0);
            int32_t ps_supplycost = tin2.getInt32(i, 2);
            int32_t s_rowid = it->second.s_rowid;
            int32_t n_rowid = it->second.n_rowid;
            tout.setInt32(r, 0, ps_partkey);
            tout.setInt32(r, 1, ps_supplycost);
            tout.setInt32(r, 2, s_rowid);
            tout.setInt32(r, 3, n_rowid);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2Join_t2_p" << std::endl;
}

// t4:747

void q2Filter_p(Table& tin1, Table& tout) {
    int nrow = tin1.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type = tin1.getcharN<char, TPCH_READ_P_TYPE_LEN + 1>(i, 2);
        int32_t p_size = tin1.getInt32(i, 3);
        // if( p_size==15&& std::regex_match(p_type.data(), std::regex("(.*)(BRASS)"))){
        if (p_size == 15 && std::string(p_type.data()).find("BRASS") == std::string(p_type.data()).length() - 5) {
            int32_t p_partkey = tin1.getInt32(i, 0);
            int32_t p_rowid = i;
            tout.setInt32(r, 0, p_partkey);
            tout.setInt32(r, 1, p_rowid);
            //  tout.setcharN<char,TPCH_READ_P_MFG_LEN+1>(r,1,p_mfgr);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2Filter_p" << std::endl;
}

// t5:642
void q2Join_t4_t3(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        int32_t p_rowid = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(p_partkey, p_rowid));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_partkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(ps_partkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t ps_supplycost = tin2.getInt32(i, 1);
            int32_t s_rowid = tin2.getInt32(i, 2);
            int32_t n_rowid = tin2.getInt32(i, 3);
            int32_t p_rowid = it->second;
            tout.setInt32(r, 0, ps_partkey);
            tout.setInt32(r, 1, ps_supplycost);
            tout.setInt32(r, 2, p_rowid);
            tout.setInt32(r, 3, s_rowid);
            tout.setInt32(r, 4, n_rowid);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2Join_t4_t3" << std::endl;
}
// t6:460
void q2GroupBy(Table& tin, Table& tout) {
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t ps_partkey = tin.getInt32(i, 0);
        int32_t ps_supplycost = tin.getInt32(i, 1);
        ;
        auto it = ht1.find(ps_partkey);
        if (it != ht1.end()) {
            int64_t s = it->second < ps_supplycost ? it->second : ps_supplycost;
            ht1[ps_partkey] = s;
        } else {
            ht1.insert(std::make_pair(ps_partkey, ps_supplycost));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        tout.setInt32(r, 0, it.first);
        tout.setInt32(r, 1, it.second);
        // std::cout<<it.first<<" "<<it.second<<std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2GroupBy" << std::endl;
}
// t7
struct KeyA {
    int32_t p_partkey;
    int32_t ps_supplycost;
    bool operator==(const KeyA& other) const {
        return (p_partkey == other.p_partkey) && (ps_supplycost == other.ps_supplycost);
    }
};
namespace std {
template <>
struct hash<KeyA> {
    std::size_t operator()(const KeyA& k) const {
        using std::size_t;
        using std::hash;
        return (hash<int>()(k.p_partkey)) + (hash<int>()(k.ps_supplycost));
    }
};
};
struct KeyV1 {
    int32_t p_rowid; // pmf
    int32_t s_rowid;
    int32_t n_rowid;
};
void q2Join_t5_t6(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<KeyA, KeyV1> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        int32_t ps_supplycost = tin1.getInt32(i, 1);
        int32_t p_rowid = tin1.getInt32(i, 2);
        int32_t s_rowid = tin1.getInt32(i, 3);
        int32_t n_rowid = tin1.getInt32(i, 4);
        //        std::cout<<p_partkey<<" "<<ps_supplycost<<std::endl;
        KeyA keya{p_partkey, ps_supplycost};
        KeyV1 keyv1{p_rowid, s_rowid, n_rowid};
        ht1.insert(std::make_pair(keya, keyv1));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_partkey = tin2.getInt32(i, 0);
        int32_t min_ps_supplycost = tin2.getInt32(i, 1);
        // std::cout<<ps_partkey<<" "<<min_ps_supplycost<<std::endl;
        auto its = ht1.equal_range(KeyA{ps_partkey, min_ps_supplycost});
        auto it = its.first;
        while (it != its.second) {
            int32_t p_rowid = it->second.p_rowid;
            int32_t s_rowid = it->second.s_rowid;
            int32_t n_rowid = it->second.n_rowid;
            tout.setInt32(r, 0, ps_partkey);
            tout.setInt32(r, 1, p_rowid);
            tout.setInt32(r, 2, s_rowid);
            tout.setInt32(r, 3, n_rowid);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2Join_t5_t6" << std::endl;
}
void q2Join_t6_t5(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_multimap<KeyA, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t ps_partkey = tin1.getInt32(i, 0);
        int32_t min_ps_supplycost = tin1.getInt32(i, 1);

        KeyA keya{ps_partkey, min_ps_supplycost};
        ht1.insert(std::make_pair(keya, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t p_partkey = tin2.getInt32(i, 0);
        int32_t ps_supplycost = tin2.getInt32(i, 1);
        int32_t p_rowid = tin2.getInt32(i, 2);
        int32_t s_rowid = tin2.getInt32(i, 3);
        int32_t n_rowid = tin2.getInt32(i, 4);
        auto its = ht1.equal_range(KeyA{p_partkey, ps_supplycost});
        auto it = its.first;
        while (it != its.second) {
            tout.setInt32(r, 0, p_partkey);
            tout.setInt32(r, 1, p_rowid);
            tout.setInt32(r, 2, s_rowid);
            tout.setInt32(r, 3, n_rowid);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q2Join_t5_t6" << std::endl;
}
void q2Sort(Table& tin, Table& p, Table& s, Table& n, Table& tout) {
    struct Q2SortRow {
        int32_t s_acctbal;
        std::string s_name;
        std::string n_name;
        int32_t p_partkey;
        std::string p_mfgr;
        // std::string s_address;
        // std::string s_phone;
        // std::string s_comment;
    };

    struct {
        // operator <
        bool operator()(const Q2SortRow& a, const Q2SortRow& b) const {
            return a.s_acctbal > b.s_acctbal || (a.s_acctbal == b.s_acctbal && a.n_name < b.n_name) ||
                   (a.s_acctbal == b.s_acctbal && a.n_name == b.n_name && a.s_name < b.s_name) ||
                   (a.s_acctbal == b.s_acctbal && a.n_name == b.n_name && a.s_name == b.s_name &&
                    a.p_partkey < b.p_partkey);
        }
    } Q2SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q2SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        int32_t p_partkey = tin.getInt32(i, 0);
        int32_t p_rowid = tin.getInt32(i, 1);
        int32_t s_rowid = tin.getInt32(i, 2);
        int32_t n_rowid = tin.getInt32(i, 3);
        std::array<char, TPCH_READ_S_NAME_LEN + 1> s_name = s.getcharN<char, TPCH_READ_S_NAME_LEN + 1>(s_rowid, 3);
        int32_t s_acctbal = s.getInt32(s_rowid, 2);
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = n.getcharN<char, TPCH_READ_NATION_LEN + 1>(n_rowid, 2);
        std::array<char, TPCH_READ_P_MFG_LEN + 1> p_mfgr = p.getcharN<char, TPCH_READ_P_MFG_LEN + 1>(p_rowid, 1);
        Q2SortRow t = {s_acctbal, std::string(s_name.data()), std::string(n_name.data()), p_partkey,
                       std::string(p_mfgr.data())};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q2SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_S_NAME_LEN + 1> s_name;
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name;
        std::array<char, TPCH_READ_P_MFG_LEN + 1> p_mfgr;
        memcpy(s_name.data(), it.s_name.data(), it.s_name.length());
        memcpy(n_name.data(), it.n_name.data(), it.n_name.length());
        memcpy(p_mfgr.data(), it.p_mfgr.data(), it.p_mfgr.length());
        tout.setInt32(r, 0, it.s_acctbal);
        tout.setcharN<char, TPCH_READ_S_NAME_LEN + 1>(r, 1, s_name);
        tout.setcharN<char, TPCH_READ_NATION_LEN + 1>(r, 2, n_name);
        tout.setInt32(r, 3, it.p_partkey);
        tout.setcharN<char, TPCH_READ_P_MFG_LEN + 1>(r, 4, p_mfgr);
        if (r < 15)
            std::cout << it.s_acctbal << " " << it.s_name << " " << it.s_name << " " << it.p_partkey << " " << it.p_mfgr
                      << std::endl;
        ++r;
    }
    tout.setNumRow(r);
}
