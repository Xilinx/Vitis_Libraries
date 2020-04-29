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
// t1:5 rows
// select count(*) from region,nation,customer where n_regionkey = r_regionkey and r_name = 'AMERICA' and c_nationkey =
// n_nationkey
void q8Join_r_n(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        std::array<char, TPCH_READ_REGION_LEN + 1> r_name = tin1.getcharN<char, TPCH_READ_REGION_LEN + 1>(i, 1);
        if (!strcmp("AMERICA", r_name.data())) {
            int32_t r_regionkey = tin1.getInt32(i, 0);
            ht1.insert(std::make_pair(r_regionkey, 0));
        }
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t n_regionkey = tin2.getInt32(i, 0);
        auto it = ht1.find(n_regionkey);
        if (it != ht1.end()) {
            int32_t n_nationkey = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, n_nationkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Join_r_n" << std::endl;
}

// t2:29952
// select count(*) from region,nation,customer where n_regionkey = r_regionkey and r_name = 'AMERICA' and c_nationkey =
// n_nationkey;
void q8Join_t1_c(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t c_nationkey = tin2.getInt32(i, 0);
        auto it = ht1.find(c_nationkey);
        if (it != ht1.end()) {
            int32_t c_custkey = tin2.getInt32(i, 1);
            tout.setInt32(r, 0, c_custkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Join_t1_c" << std::endl;
}

// t3:91179
// select count(*) from region,nation,customer,orders  where n_regionkey = r_regionkey and r_name = 'AMERICA' and
// c_nationkey = n_nationkey and o_custkey = c_custkey  and o_orderdate between date '1995-01-01' and date '1996-12-31';
void q8Join_t2_o(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    std::cout << std::dec << nrow1 << " " << nrow2 << std::endl;
    for (int i = 0; i < nrow1; i++) {
        int32_t c_custkey = tin1.getInt32(i, 0);
        ht1.insert(std::make_pair(c_custkey, 0));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        //        std::cout<<i<<std::endl;
        int32_t o_orderdate = tin2.getInt32(i, 2);
        if (o_orderdate >= 19950101 && o_orderdate <= 19961231) {
            int32_t o_custkey = tin2.getInt32(i, 0);
            int32_t o_orderkey = tin2.getInt32(i, 1);
            auto it = ht1.find(o_custkey);
            if (it != ht1.end()) {
                tout.setInt32(r, 0, o_orderkey);
                tout.setInt32(r, 1, o_orderdate);
                // if(r<10) std::cout<<std::dec<<tout.getInt32(r,0)<<" "<<tout.getInt32(r,1)<<std::endl;
                r++;
            }
        }
    }

    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Join_t2_o" << std::endl;
}

// t4
// select count(*) from region,nation,customer,orders,lineitem  where n_regionkey = r_regionkey and r_name = 'AMERICA'
// and c_nationkey = n_nationkey and o_custkey = c_custkey  and o_orderdate between date '1995-01-01' and date
// '1996-12-31' and o_orderkey=l_orderkey;
// count
// --------
//  365091
//  (1 row)
//
void q8Join_t3_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t o_orderdate = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(o_orderkey, o_orderdate));
    }

    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        auto it = ht1.find(l_orderkey);
        if (it != ht1.end()) {
            int32_t l_partkey = tin2.getInt32(i, 1);
            int32_t l_suppkey = tin2.getInt32(i, 2);
            int32_t l_extendedprice = tin2.getInt32(i, 3);
            int32_t l_discount = tin2.getInt32(i, 4);
            int32_t o_orderdate = it->second;
            int32_t e = l_extendedprice * (100 - l_discount);
            tout.setInt32(r, 0, l_partkey);
            tout.setInt32(r, 1, l_suppkey);
            tout.setInt32(r, 2, e);
            tout.setInt32(r, 3, o_orderdate);
            // if(r<10) std::cout<<std::dec<<tout.getInt32(r,0)<<std::endl;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Join_t3_l" << std::endl;
}
// t5:1451
// select count(*) from part where p_type = 'ECONOMY ANODIZED STEEL';
void q8Filter_p(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_TYPE_LEN + 1> p_type = tin.getcharN<char, TPCH_READ_P_TYPE_LEN + 1>(i, 1);
        if (!strcmp("ECONOMY ANODIZED STEEL", p_type.data())) {
            int32_t p_partkey = tin.getInt32(i, 0);
            tout.setInt32(r, 0, p_partkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Filter_p" << std::endl;
}

// t7:2603
// select count(*) from part,region,nation,customer,orders,lineitem  where n_regionkey = r_regionkey and r_name =
// 'AMERICA' and c_nationkey = n_nationkey and o_custkey = c_custkey  and o_orderdate between date '1995-01-01' and date
// '1996-12-31' and o_orderkey=l_orderkey and p_type = 'ECONOMY ANODIZED STEEL' and p_partkey = l_partkey;
void q8Join_t5_t4(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t p_partkey = tin1.getInt32(i, 0);
        ht1.insert(std::make_pair(p_partkey, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_partkey = tin2.getInt32(i, 0);
        auto it = ht1.find(l_partkey);
        if (it != ht1.end()) {
            int32_t l_suppkey = tin2.getInt32(i, 1);
            int32_t e = tin2.getInt32(i, 2);
            int32_t o_orderdate = tin2.getInt32(i, 3);
            tout.setInt32(r, 0, l_suppkey);
            tout.setInt32(r, 1, e);
            tout.setInt32(r, 2, o_orderdate);
            if (r < 10) std::cout << std::dec << l_suppkey << " " << o_orderdate << std::endl;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Join_t5_t4" << std::endl;
}

// t7:2603
void q8Join_s_t6(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t s_suppkey = tin1.getInt32(i, 0);
        int32_t s_nationkey = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(s_suppkey, s_nationkey));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_suppkey = tin2.getInt32(i, 0);
        auto it = ht1.find(l_suppkey);
        if (it != ht1.end()) {
            int32_t s_nationkey = it->second;
            int32_t e = tin2.getInt32(i, 1);
            int32_t o_orderdate = tin2.getInt32(i, 2);
            tout.setInt32(r, 0, s_nationkey);
            tout.setInt32(r, 1, e);
            tout.setInt32(r, 2, o_orderdate);
            if (r < 10) std::cout << std::dec << s_nationkey << " " << o_orderdate << std::endl;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Join_s_t6" << std::endl;
}
// t8:2603
void q8Join_n_t7(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t n_nationkey = tin1.getInt32(i, 1);
        // int32_t n_rowid = i;//tin1.getInt32(i,1);
        int32_t n_rowid = tin1.getInt32(i, 3);
        ht1.insert(std::make_pair(n_nationkey, n_rowid));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t s_nationkey = tin2.getInt32(i, 0);
        auto it = ht1.find(s_nationkey);
        if (it != ht1.end()) {
            int32_t e = tin2.getInt32(i, 1);
            int32_t o_orderdate = tin2.getInt32(i, 2);
            int32_t n_rowid = it->second;
            tout.setInt32(r, 0, e);
            tout.setInt32(r, 1, o_orderdate / 10000);
            tout.setInt32(r, 2, n_rowid);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Join_n_t7" << std::endl;
}
// for siyang tow group_bys
void q8GroupBy(Table& tin, Table& tout) {
    std::unordered_map<int32_t, int64_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t e = tin.getInt32(i, 0);
        int32_t o_year = tin.getInt32(i, 1);
        auto it = ht1.find(o_year);
        if (it != ht1.end()) {
            int64_t s = it->second + e;
            ht1[o_year] = s;
        } else {
            ht1.insert(std::make_pair(o_year, e));
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

void q8GroupBy_filtern(Table& tin, Table& origin, Table& tout) {
    std::unordered_map<int32_t, int64_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t e = tin.getInt32(i, 0);
        int32_t o_year = tin.getInt32(i, 1);
        int32_t n_rowid = tin.getInt32(i, 2);
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = origin.getcharN<char, TPCH_READ_NATION_LEN + 1>(n_rowid, 2);
        if (strcmp("BRAZIL", n_name.data())) {
            e = 0;
        }
        auto it = ht1.find(o_year);
        if (it != ht1.end()) {
            int64_t s = it->second + e;
            ht1[o_year] = s;
        } else {
            ht1.insert(std::make_pair(o_year, e));
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

// for qianqiao one group by:2
void q8GroupBy_all(Table& tin, Table& origin, Table& tout) {
    struct Values {
        int64_t all;
        int64_t filter;
    };

    std::unordered_map<int32_t, Values> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t e = tin.getInt32(i, 0);
        int32_t e0 = e;
        int32_t o_year = tin.getInt32(i, 1);
        int32_t n_rowid = tin.getInt32(i, 2);
        std::array<char, TPCH_READ_NATION_LEN + 1> n_name = origin.getcharN<char, TPCH_READ_NATION_LEN + 1>(n_rowid, 2);
        if (strcmp("BRAZIL", n_name.data())) {
            e0 = 0;
        }
        auto it = ht1.find(o_year);
        if (it != ht1.end()) {
            int64_t s = it->second.all + e;
            int64_t s0 = it->second.filter + e0;
            ht1[o_year] = Values{s, s0};
        } else {
            ht1.insert(std::make_pair(o_year, Values{e, e0}));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        tout.setInt32(r, 0, it.first);
        double result = (double)it.second.filter / (double)it.second.all;
        // tout.setInt64(r, 1, it.second.filter/it.second.all);
        if (r < 10) std::cout << tout.getInt32(r, 0) << " " << result << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q8Group" << std::endl;
}
void getRe(Table& tin1, Table& tin2, Table& tout) {}
