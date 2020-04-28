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
// n_nationkey:3
void NationFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = tin.getcharN<char, TPCH_READ_NATION_LEN + 1>(i, 1);
        if (!strcmp("GERMANY", n_name.data())) {
            int32_t n_nationkey = tin.getInt32(i, 0);
            tout.setInt32(r, 0, n_nationkey);
            // tout.setcharN<char,TPCH_READ_NATION_LEN + 1>(r,1,n_name);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In NationFilter" << std::endl;
}
// 396
// select count(*) from nation,supplier where n_name = 'GERMANY' and s_nationkey = n_nationkey;
void q11Join_t1_s(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t s_nationkey = tin2.getInt32(i, 0);
        auto it = ht1.find(s_nationkey);
        if (it != ht1.end()) {
            int32_t s_suppkey = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, s_suppkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q11Join_t1_s" << std::endl;
}
// mytpch=# select count(*) from partsupp,supplier,nation where n_name = 'GERMANY' and s_nationkey = n_nationkey and
// ps_suppkey = s_suppkey;
// count
//-------
// 31680

void q11Join_t2_p(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t ps_suppkey = tin2.getInt32(i, 0);
        auto it = ht1.find(ps_suppkey);
        if (it != ht1.end()) {
            int32_t ps_partkey = tin2.getInt32(i, 1);
            int32_t ps_supplycost = tin2.getInt32(i, 2);
            int32_t ps_availqty = tin2.getInt32(i, 3);
            int32_t e = ps_supplycost * ps_availqty;
            tout.setInt32(r, 0, ps_partkey);
            tout.setInt32(r, 1, e);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q11Join_t2_p" << std::endl;
}
// select sum(ps_supplycost*ps_availqty)*0.0001 from partsupp,supplier,nation where n_name = 'GERMANY' and s_nationkey =
// n_nationkey and ps_suppkey = s_suppkey;
// 7874103.109405
int64_t sq11Sum(Table& tin1) {
    int nrow1 = tin1.getNumRow();
    int64_t sum = 0;
    for (int i = 0; i < nrow1; i++) {
        int32_t e = tin1.getInt32(i, 1);
        sum += e;
    }
    return sum;
}
int64_t q14scalsum(Table& tin) {
    int64_t sum = sq11Sum(tin);
    return sum * 0.0001;
}
// 2981
// select count(*) from (select  sum(ps_supplycost*ps_availqty) from partsupp,supplier,nation where n_name = 'GERMANY'
// and s_nationkey = n_nationkey and ps_suppkey = s_suppkey group by ps_partkey)ff;
void q11Groupby(Table& tin1, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t ps_partkey = tin1.getInt32(i, 0);
        int32_t e = tin1.getInt32(i, 1);
        auto it = ht1.find(ps_partkey);
        if (it != ht1.end()) {
            int64_t t = it->second + e;
            ht1[ps_partkey] = t;
        } else {
            ht1.insert(std::make_pair(ps_partkey, e));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        tout.setInt32(r, 0, it.first);
        tout.setInt64(r, 1, it.second);
        ++r;
    }
    tout.setNumRow(r);
}

void q11Filter_Sort(int64_t filterv, Table& tin, Table& tout) {
    std::cout << std::dec << filterv << " In q11Filter_Sort" << std::endl;
    struct Q11SortRow {
        int32_t ps_partkey;
        int64_t sum;
    };

    struct {
        // operator <
        bool operator()(const Q11SortRow& a, const Q11SortRow& b) const {
            return a.sum > b.sum || (a.sum == b.sum && a.ps_partkey < b.ps_partkey);
        }
    } Q11SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q11SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        int64_t sum = tin.getInt64(i, 1);
        if (sum > filterv) {
            int32_t ps_partkey = tin.getInt32(i, 0);
            Q11SortRow t = {ps_partkey, sum};
            rows.push_back(t);
        }
    }

    std::sort(rows.begin(), rows.end(), Q11SortLess);

    int r = 0;
    for (auto& it : rows) {
        tout.setInt32(r, 0, it.ps_partkey);
        tout.setInt64(r, 1, it.sum);
        if (r < 10) std::cout << std::dec << tout.getInt32(r, 0) << " " << tout.getInt64(r, 1) << std::endl;
        ++r;
    }
    tout.setNumRow(r);
}
// t4
