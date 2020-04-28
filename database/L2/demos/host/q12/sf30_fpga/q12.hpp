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
// t1:1715437
void LineFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    std::cout << std::dec << nrow << " " << std::endl;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> l_shipmode = tin.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(i, 1);
        //    if(i<10) std::cout<< l_shipmode.data()<<std::endl;
        if (!strcmp(l_shipmode.data(), "MAIL") || !strcmp(l_shipmode.data(), "SHIP")) {
            int32_t l_orderkey = tin.getInt32(i, 0);
            int32_t l_commitdate = tin.getInt32(i, 2);
            int32_t l_receiptdate = tin.getInt32(i, 3);
            int32_t l_shipdate = tin.getInt32(i, 4);
            int32_t l_rowid = i;
            tout.setInt32(r, 0, l_orderkey);
            tout.setInt32(r, 1, l_rowid);
            // tout.setcharN<char,TPCH_READ_MAXAGG_LEN + 1>(r,1,l_shipmode);
            tout.setInt32(r, 2, l_commitdate);
            tout.setInt32(r, 3, l_receiptdate);
            tout.setInt32(r, 4, l_shipdate);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " in LineFilter" << std::endl;
}
// o_orderpriority in col3
// 30988
void q12Join_o_t1(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t o_rowid = tin1.getInt32(i, 1);
        ht1.insert(std::make_pair(o_orderkey, o_rowid));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        int32_t l_commitdate = tin2.getInt32(i, 2);
        int32_t l_receiptdate = tin2.getInt32(i, 3);
        int32_t l_shipdate = tin2.getInt32(i, 4);
        auto it = ht1.find(l_orderkey);
        if (l_commitdate < l_receiptdate && l_shipdate < l_commitdate && l_receiptdate >= 19940101 &&
            l_receiptdate < 19950101 && it != ht1.end()) {
            int32_t l_rowid = tin2.getInt32(i, 1);
            int32_t o_rowid = it->second;
            tout.setInt32(r, 0, o_rowid);
            tout.setInt32(r, 1, l_rowid);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " in q12Join_o_t1" << std::endl;
}
// t2
struct q12GroupKey {
    std::string l_shipmode;
    bool operator==(const q12GroupKey& other) const { return (l_shipmode == other.l_shipmode); }
};
namespace std {
template <>
struct hash<q12GroupKey> {
    std::size_t operator()(const q12GroupKey& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<string>()(k.l_shipmode));
    }
};
}
struct sumVs {
    int64_t sum1;
    int64_t sum2;
};

void q12Groupby(Table* tin1, Table& origint1, Table& origint2, Table& tout, int hjTimes) {
    std::unordered_map<q12GroupKey, sumVs> ht1;
    for (int p = 0; p < hjTimes; p++) {
        int nrow1 = tin1[p].getNumRow();
        for (int i = 0; i < nrow1; i++) {
            int32_t l_rowid = tin1[p].getInt32(i, 1);
            std::array<char, TPCH_READ_MAXAGG_LEN + 1> l_shipmode =
                origint1.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(l_rowid, 1);
            int32_t o_rowid = tin1[p].getInt32(i, 0);
            std::array<char, TPCH_READ_MAXAGG_LEN + 1> o_orderpriority =
                origint2.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(o_rowid, 2);
            q12GroupKey k{std::string(l_shipmode.data())};
            auto it = ht1.find(k);
            if (it != ht1.end()) {
                if (!strcmp(o_orderpriority.data(), "1-URGENT") || !strcmp(o_orderpriority.data(), "2-HIGH")) {
                    int64_t t = it->second.sum1 + 1;
                    ht1[k].sum1 = t;
                } else {
                    int64_t t = it->second.sum2 + 1;
                    ht1[k].sum2 = t;
                }

            } else {
                if (!strcmp(o_orderpriority.data(), "1-URGENT") || !strcmp(o_orderpriority.data(), "2-HIGH")) {
                    ht1.insert(std::make_pair(k, sumVs{1, 0}));
                } else {
                    ht1.insert(std::make_pair(k, sumVs{0, 1}));
                }
            }
        }
    }

    int r = 0;
    for (auto& it : ht1) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> l_shipmode;
        memcpy(l_shipmode.data(), (it.first.l_shipmode).data(), (it.first.l_shipmode).length());
        tout.setcharN<char, TPCH_READ_MAXAGG_LEN + 1>(r, 0, l_shipmode);
        tout.setInt64(r, 1, it.second.sum1);
        tout.setInt64(r, 2, it.second.sum2);
        r++;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " in q12Groupby1" << std::endl;
}

void q12Sort(Table& tin, Table& tout) {
    struct Q12SortRow {
        std::string l_shipmode;
        int64_t sum1;
        int64_t sum2;
    };

    struct {
        bool operator()(const Q12SortRow& a, const Q12SortRow& b) const { return a.l_shipmode < b.l_shipmode; }
    } Q12SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q12SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> l_shipmode = tin.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(i, 0);
        int64_t sum1 = tin.getInt64(i, 1);
        int64_t sum2 = tin.getInt64(i, 2);
        Q12SortRow t = {std::string(l_shipmode.data()), sum1, sum2};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q12SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> l_shipmode;
        std::string l_shipmode_ = it.l_shipmode;
        memcpy(l_shipmode.data(), l_shipmode_.data(), l_shipmode_.length());
        tout.setcharN<char, TPCH_READ_MAXAGG_LEN + 1>(r, 0, l_shipmode);
        tout.setInt64(r, 1, it.sum1);
        tout.setInt64(r, 2, it.sum2);
        if (r < 10)
            std::cout << std::dec << tout.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(r, 0).data() << " "
                      << tout.getInt64(r, 1) << " " << tout.getInt64(r, 2) << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " in q12Sort" << std::endl;
}
// t4
// l_shipmode | high_line_count | low_line_count
// ------------+-----------------+----------------
//  MAIL       |            6202 |           9324
//   SHIP       |            6200 |           9262
//   (2 rows)
//
