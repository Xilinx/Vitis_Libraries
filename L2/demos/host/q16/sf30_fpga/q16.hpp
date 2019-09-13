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
void q16Filter_p(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 1);
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type = tin.getcharN<char, TPCH_READ_P_TYPE_LEN + 1>(i, 2);
        // if(!std::regex_match(p_type.data(), std::regex("(MEDIUM POLISHED)(.*)"))&&strcmp(p_brand.data(),"Brand#45")){
        if (strcmp(p_brand.data(), "Brand#45") && std::string(p_type.data()).find("MEDIUM POLISHED") != 0) {
            // if(std::string(p_type.data()).find("MEDIUM POLISHED")!=0&&strcmp(p_brand.data(),"Brand#45")){
            int32_t p_partkey = tin.getInt32(i, 0);
            int32_t p_size = tin.getInt32(i, 3);
            int32_t p_rowid = i; // tin.getInt32(i,4);
            tout.setInt32(r, 0, p_partkey);
            tout.setInt32(r, 1, p_size);
            tout.setInt32(r, 2, p_rowid);
            //            tout.setcharN<char,TPCH_READ_P_BRND_LEN+1>(r,3,p_brand);
            //            tout.setcharN<char,TPCH_READ_P_TYPE_LEN+1>(r,4,p_type);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q16Filter_p" << std::endl;
}

void q16Join_c_t1(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();

    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    std::cout << "build: " << nrow1 << " probe: " << nrow2 << std::endl;
    for (int i = 0; i < nrow2; i++) {
        int32_t p_size = tin2.getInt32(i, 1);
        auto it = ht1.find(p_size);
        if (it != ht1.end()) {
            int32_t p_partkey = tin2.getInt32(i, 0);
            int32_t p_rowid = tin2.getInt32(i, 2);
            tout.setInt32(r, 0, p_partkey);
            tout.setInt32(r, 1, p_rowid);
            tout.setInt32(r, 2, p_size);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q16Join_c_t1" << std::endl;
}

bool strm_pattern(std::string sub1, std::string sub2, std::string s, int len = 7) {
    std::string::size_type spe_f = s.find(sub1);
    std::string::size_type c_f = 0;
    while (spe_f != std::string::npos) {
        c_f += (spe_f + len);
        std::string sub_s = s.substr(c_f);
        if (sub_s.find(sub2) != std::string::npos) return true;
        spe_f = sub_s.find(sub1);
    }
    return false;
}
// t3
void q16Filter_s(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_S_CMNT_MAX + 1> s_comment = tin.getcharN<char, TPCH_READ_S_CMNT_MAX + 1>(i, 1);
        // if(!std::regex_match (s_comment.data(), std::regex("(.*)(Customer)(.*)(Complaint)(.*)"))){
        if (!strm_pattern("Customer", "Complaint", s_comment.data(), 8)) {
            int32_t s_suppkey = tin.getInt32(i, 0);
            // std::cout<<s_suppkey<<"   "<<s_comment.data()<<std::endl;
            tout.setInt32(r, 0, s_suppkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q16Filter_s" << std::endl;
}

void q16Join_t3_p(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::cout << "build: " << nrow1 << " probe: " << nrow2 << std::endl;
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_suppkey = tin1.getInt32(i, 0);
        ht1.insert(std::make_pair(s_suppkey, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_suppkey = tin2.getInt32(i, 0);
        auto it = ht1.find(ps_suppkey);
        if (it != ht1.end()) {
            int32_t ps_partkey = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, ps_partkey);
            tout.setInt32(r, 1, ps_suppkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q16Join_t3_p" << std::endl;
}

// t4
void q16Join_t4_t2(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::cout << "build: " << nrow1 << " probe: " << nrow2 << std::endl;
    std::unordered_multimap<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t ps_partkey = tin1.getInt32(i, 0);
        int32_t ps_suppkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(ps_partkey, ps_suppkey));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t p_partkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(p_partkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t p_rowid = tin2.getInt32(i, 1);
            int32_t p_size = tin2.getInt32(i, 2);
            int32_t ps_suppkey = it->second;
            tout.setInt32(r, 0, p_rowid);
            tout.setInt32(r, 1, p_size);
            tout.setInt32(r, 2, ps_suppkey);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q16Join_t4_t2" << std::endl;
}
struct Q16GroupKey {
    std::string p_brand;
    std::string p_type;
    int32_t p_size;
    // std::array

    bool operator==(const Q16GroupKey& other) const {
        return (p_brand == other.p_brand) && (p_type == other.p_type) && (p_size == other.p_size);
    }
};
namespace std {

template <>
struct hash<Q16GroupKey> {
    std::size_t operator()(const Q16GroupKey& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<string>()(k.p_brand)) + (hash<string>()(k.p_type)) + (hash<int>()(k.p_size));
    }
};
}

void q16GroupBy(Table& tin, Table& origin, Table& tout) {
    std::unordered_map<Q16GroupKey, std::set<int> > m;
    unsigned nrow = tin.getNumRow();
    std::cout << "build: " << nrow << std::endl;
    for (size_t i = 0; i < nrow; ++i) {
        int32_t p_rowid = tin.getInt32(i, 0); // index much patMatch YAML
        int32_t p_size = tin.getInt32(i, 1);  // index much patMatch YAML
        int32_t ps_suppkey = tin.getInt32(i, 2);
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand =
            origin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(p_rowid, 1);
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type = origin.getcharN<char, TPCH_READ_P_TYPE_LEN + 1>(p_rowid, 2);
        Q16GroupKey k{std::string(p_brand.data()), std::string(p_type.data()), p_size};
        auto it = m.find(k);
        if (it != m.end()) {
            m[k].insert(ps_suppkey); // update
        } else {
            std::set<int> myset;
            myset.insert(ps_suppkey);
            m.insert(std::make_pair(k, myset));
        }
    }
    int r = 0;
    for (auto& it : m) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand_s{};
        memcpy(p_brand_s.data(), (it.first.p_brand).data(), (it.first.p_brand).length());
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type_s{};
        memcpy(p_type_s.data(), (it.first.p_type).data(), (it.first.p_type).length());
        tout.setcharN<char, TPCH_READ_P_BRND_LEN + 1>(r, 0, p_brand_s);
        tout.setcharN<char, TPCH_READ_P_TYPE_LEN + 1>(r, 1, p_type_s);
        tout.setInt32(r, 2, it.first.p_size);
        tout.setInt64(r, 3, it.second.size());
        // if(r<10)std::cout<<tout.getcharN<char,TPCH_READ_P_BRND_LEN+1>(r, 0).data()<<"
        // "<<tout.getcharN<char,TPCH_READ_P_TYPE_LEN+1>(r, 1).data()<<" "<<std::dec<<tout.getInt32(r,2)<<std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " groupby_q16" << std::endl;
}

void q16Sort(Table& tin, Table& tout) {
    struct Q16SortRow {
        int64_t distc_cn;
        std::string p_brand;
        std::string p_type;
        int32_t p_size;
    };

    struct {
        // operator <
        bool operator()(const Q16SortRow& a, const Q16SortRow& b) const {
            return a.distc_cn > b.distc_cn || (a.distc_cn == b.distc_cn && a.p_brand < b.p_brand) ||
                   (a.distc_cn == b.distc_cn && a.p_brand == b.p_brand && a.p_type < b.p_type) ||
                   (a.distc_cn == b.distc_cn && a.p_brand == b.p_brand && a.p_type == b.p_type && a.p_size < b.p_size);
        }
    } Q16SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q16SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 0);
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type = tin.getcharN<char, TPCH_READ_P_TYPE_LEN + 1>(i, 1);
        Q16SortRow t = {tin.getInt64(i, 3), std::string(p_brand.data()), std::string(p_type.data()),
                        tin.getInt32(i, 2)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q16SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand_s{};
        memcpy(p_brand_s.data(), (it.p_brand).data(), (it.p_brand).length());
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type_s{};
        memcpy(p_type_s.data(), (it.p_type).data(), (it.p_type).length());
        tout.setcharN<char, TPCH_READ_P_BRND_LEN + 1>(r, 0, p_brand_s);
        tout.setcharN<char, TPCH_READ_P_TYPE_LEN + 1>(r, 1, p_type_s);
        tout.setInt32(r, 2, it.p_size);
        tout.setInt64(r, 3, it.distc_cn);
        if (r < 10)
            std::cout << tout.getInt64(r, 3) << tout.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(r, 0).data() << "  "
                      << tout.getcharN<char, TPCH_READ_P_TYPE_LEN + 1>(r, 1).data() << " " << std::dec
                      << tout.getInt32(r, 2) << std::endl;
        ++r;
    }
    tout.setNumRow(r);
}
