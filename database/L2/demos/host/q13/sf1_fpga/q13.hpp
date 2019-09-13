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
// 1479964 t1
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
void OrderFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_O_CMNT_MAX + 1> o_comment = tin.getcharN<char, TPCH_READ_O_CMNT_MAX + 1>(i, 2);
        // if(!std::regex_match(o_comment.data(), std::regex("(.*)(special)(.*)(requests)(.*)"))){
        if (!strm_pattern("special", "requests", o_comment.data())) {
            int32_t o_custkey = tin.getInt32(i, 0);
            int32_t o_orderkey = tin.getInt32(i, 1);
            tout.setInt32(r, 0, o_custkey);
            tout.setInt32(r, 1, o_orderkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after OrderFilter" << std::endl;
}

// 1479964 t2
void q13Join_c_t1(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    //    std::cout<<std::dec<<nrow1<<" "<<nrow2<<std::endl;
    for (int i = 0; i < nrow2; i++) {
        int32_t o_custkey = tin2.getInt32(i, 0);
        auto it = ht1.find(o_custkey);
        if (it != ht1.end()) {
            int32_t o_orderkey = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, o_custkey);
            tout.setInt32(r, 1, o_orderkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q13Join_c_t1" << std::endl;
}

// t3
void q13AntiJoin_t2_c(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_custkey = tin1.getInt32(i, 0);
        int32_t o_orderkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(o_custkey, o_orderkey));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t c_custkey = tin2.getInt32(i, 0);
        auto it = ht1.find(c_custkey);
        if (it == ht1.end()) {
            tout.setInt32(r, 0, c_custkey);
            tout.setInt32(r, 1, 0);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q13AntiJoin_t2_c" << std::endl;
}
void q13GroupBy_t2(Table& tin, Table& tout) {
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t c_custkey = tin.getInt32(i, 0);
        auto it = ht1.find(c_custkey);
        if (it != ht1.end()) {
            int64_t s = it->second + 1;
            ht1[c_custkey] = s;
        } else {
            ht1.insert(std::make_pair(c_custkey, 1));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        tout.setInt32(r, 0, it.first);
        tout.setInt32(r, 1, it.second);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q13GroupBy3" << std::endl;
}
void q13GroupBy_t3(Table& tin, Table& tout) {
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t o_custkey = tin.getInt32(i, 0);
        auto it = ht1.find(o_custkey);
        if (it == ht1.end()) {
            ht1.insert(std::make_pair(o_custkey, 0));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        tout.setInt32(r, 0, it.first);
        tout.setInt32(r, 1, 0);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q13GroupBy2" << std::endl;
}
// t4:150000
void combile_t4_t5(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    int i = 0;
    for (i = 0; i < nrow1; i++) {
        tout.setInt32(i, 0, tin1.getInt32(i, 0));
        tout.setInt32(i, 1, tin1.getInt32(i, 8));
    }
    for (i = nrow1; i < nrow1 + nrow2; i++) {
        int j = i - nrow1;
        tout.setInt32(i, 0, tin2.getInt32(j, 0));
        tout.setInt32(i, 1, tin2.getInt32(j, 1));
    }
    tout.setNumRow(i);
    std::cout << std::dec << i << " after combile_t4_t5" << std::endl;
}
void q13GroupBy(Table& tin, Table& tout) {
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t c_count = tin.getInt32(i, 1);
        auto it = ht1.find(c_count);
        if (it != ht1.end()) {
            int32_t s = it->second + 1;
            ht1[c_count] = s;
        } else {
            ht1.insert(std::make_pair(c_count, 1));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        tout.setInt32(r, 0, it.first);
        tout.setInt32(r, 1, it.second);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q13GroupBy3" << std::endl;
}
void q13Sort(Table& tin, Table& tout) {
    struct Q13SortRow {
        int32_t c_count;
        int32_t custdist;
    };
    struct {
        // operator <
        bool operator()(const Q13SortRow& a, const Q13SortRow& b) const {
            return a.custdist > b.custdist || (a.custdist == b.custdist && a.c_count > b.c_count);
        }
    } Q13SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q13SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        Q13SortRow t = {tin.getInt32(i, 0), tin.getInt32(i, 1)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q13SortLess);

    int r = 0;
    for (auto& it : rows) {
        tout.setInt32(r, 0, it.c_count);
        tout.setInt32(r, 1, it.custdist);
        std::cout << it.c_count << " " << it.custdist << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q13Sort" << std::endl;
}
