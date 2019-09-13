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
// order and lineitem

void q15GroupBy(Table& tin, Table& tout) {
    std::unordered_map<int32_t, int64_t> m;
    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int32_t l_shipdate = tin.getInt32(i, 3);
        //        if(i<10) std::cout<<std::dec<<l_shipdate<<std::endl;
        if (l_shipdate >= 19960101 && l_shipdate < 19960401) {
            int32_t l_suppkey = tin.getInt32(i, 0);       // index much patMatch YAML
            int32_t l_extendedprice = tin.getInt32(i, 1); // index much patMatch YAML
            int32_t l_discount = tin.getInt32(i, 2);
            int32_t revenue = l_extendedprice * (100 - l_discount);
            auto it = m.find(l_suppkey);
            if (it != m.end()) {
                int64_t s = it->second + revenue;
                m[l_suppkey] = s; // update
            } else {
                m.insert(std::make_pair(l_suppkey, revenue));
            }
        }
    }
    int r = 0;
    for (auto& it : m) {
        int64_t sumv = it.second;
        tout.setInt32(r, 0, it.first);
        tout.setInt64(r, 1, sumv);
        /*  if(sumv>maxv)
             maxv = sumv;*/
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q15GroupBy" << std::endl;
}

int64_t FindMax(Table& tin) {
    int64_t maxv = 0;
    int32_t l_suppkey;

    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int64_t sum = tin.combineInt64(i, 8, 0);
        if (sum > maxv) {
            maxv = sum;
            l_suppkey = tin.getInt32(i, 7);
        }
    }
    std::cout << std::dec << "max:" << maxv << " supp_key:" << l_suppkey << std::endl;
    return maxv;
}

// the maxv cant pass into kernel
void q15_filter(Table& tin1, Table& tout, int64_t maxv) {
    int nrow1 = tin1.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow1; ++i) {
        int32_t l_suppkey = tin1.getInt32(i, 7);
        int64_t eval = tin1.combineInt64(i, 8, 0);
        if (eval == maxv) {
            tout.setInt32(r, 0, l_suppkey);
            tout.setInt64_l(r, 1, eval);
            tout.setInt64_h(r, 2, eval);
            r++;
        }
        /*
                  if(i<10){
        std::cout<<"test eval:";
                     for(int j=0;j<16;j++){
                       std::cout<<"col"<<j<<"="<<tin1.getInt32(i,j)<<" ";
                     }
        std::cout<<std::endl;
                   }
        */
    }
    // tout.setInt32(0,0,8449);
    // tout.setInt64_l(0,1,17726272087);
    // tout.setInt64_h(0,2,17726272087);

    tout.setNumRow(r);
    std::cout << std::dec << nrow1 << " out q15_groupby" << std::endl;
    std::cout << std::dec << r << " out q15_filter" << std::endl;
}
void q15Join_t1_s(Table& tin1, Table& tin2, Table& tout) {
    std::unordered_multimap<int32_t, int64_t> ht1;
    int nrow1 = tin1.getNumRow();
    for (int i = 0; i < nrow1; ++i) {
        int32_t l_suppkey = tin1.getInt32(i, 0);
        int32_t eval_l = tin1.getInt32(i, 1);
        int32_t eval_h = tin1.getInt32(i, 2);
        int64_t eval = tin1.mergeInt64(eval_l, eval_h);
        //       std::cout<<"test eval:"<<eval_l<<" "<<eval_h<<std::endl;
        ht1.insert(std::make_pair(l_suppkey, eval));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int s_suppkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(s_suppkey);
        auto it = its.first;
        while (it != its.second) {
            int s_rowid = tin2.getInt32(i, 4);
            int64_t eval = it->second;
            tout.setInt32(r, 0, s_suppkey);
            tout.setInt64_l(r, 1, eval);
            tout.setInt64_h(r, 2, eval);
            tout.setInt32(r, 3, s_rowid);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q15Join_t1_s" << std::endl;
}
void q15Sort(Table& tin, Table& origin, Table& tout) {
    struct Q15SortRow {
        int32_t s_suppkey;
        int64_t eval;
        int32_t s_rowid;
    };

    struct {
        // operator <
        bool operator()(const Q15SortRow& a, const Q15SortRow& b) const {
            return a.s_suppkey < b.s_suppkey || (a.s_suppkey == b.s_suppkey && a.eval < b.eval);
        }
    } Q15SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q15SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        int32_t eval_l = tin.getInt32(i, 1);
        int32_t eval_h = tin.getInt32(i, 2);
        int64_t eval = tin.mergeInt64(eval_l, eval_h);
        Q15SortRow t = {tin.getInt32(i, 0), eval, tin.getInt32(i, 3)};
        //        std::cout<<tin.getInt32(i, 0)<<" "<<tin.getInt64(i, 1)<<" "<<tin.getInt32(i, 2)<<std::endl;
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q15SortLess);

    int r = 0;
    for (auto& it : rows) {
        int32_t s_rowid = it.s_rowid;
        std::array<char, TPCH_READ_S_NAME_LEN + 1> s_name = origin.getcharN<char, TPCH_READ_S_NAME_LEN + 1>(s_rowid, 1);
        std::array<char, TPCH_READ_S_ADDR_MAX + 1> s_address =
            origin.getcharN<char, TPCH_READ_S_ADDR_MAX + 1>(s_rowid, 2);
        std::array<char, TPCH_READ_PHONE_LEN + 1> s_phone = origin.getcharN<char, TPCH_READ_PHONE_LEN + 1>(s_rowid, 3);
        tout.setInt32(r, 0, it.s_suppkey);
        tout.setcharN<char, TPCH_READ_S_NAME_LEN + 1>(r, 1, s_name);
        tout.setcharN<char, TPCH_READ_S_ADDR_MAX + 1>(r, 2, s_address);
        tout.setcharN<char, TPCH_READ_PHONE_LEN + 1>(r, 3, s_phone);
        tout.setInt64(r, 4, it.eval);
        if (r < 10)
            std::cout << std::dec << it.s_suppkey << " " << s_name.data() << " " << s_address.data() << " "
                      << s_phone.data() << " " << it.eval << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q15Sort" << std::endl;
}

// CREATE VIEW
// s_suppkey |          s_name           |     s_address     |     s_phone     | total_revenue
// -----------+---------------------------+-------------------+-----------------+---------------
//       8449 | Supplier#000008449        | 5BXWsJERA2mP5OyO4 | 20-469-856-8873 |  1772627.2087
//       (1 row)
//
