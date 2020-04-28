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
void q4SemiJoin_o_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    std::cout << std::dec << nrow1 << " " << std::endl;
    for (int i = 0; i < nrow1; i++) {
        int32_t l_commitdate = tin1.getInt32(i, 1);
        int32_t l_receiptdate = tin1.getInt32(i, 2);
        int32_t l_orderkey = tin1.getInt32(i, 0);
        if (l_commitdate < l_receiptdate) {
            ht1.insert(std::make_pair(l_orderkey, 0));
        }
    }
    // std::cout<<std::dec<<ht1.size()<<" In q4"<<std::endl;
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t o_orderkey = tin2.getInt32(i, 1);
        int32_t o_orderdate = tin2.getInt32(i, 2);
        if ((o_orderdate >= 19930701 && o_orderdate < 19931001) && ht1.find(o_orderkey) != ht1.end()) {
            int32_t o_rowid = tin2.getInt32(i, 3);
            tout.setInt32(r, 0, o_rowid);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q4SemiJoin_o_l" << std::endl;
}

struct q4GroupKey {
    std::string o_orderpriority;
    bool operator==(const q4GroupKey& other) const { return (o_orderpriority == other.o_orderpriority); }
};
namespace std {
template <>
struct hash<q4GroupKey> {
    std::size_t operator()(const q4GroupKey& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<string>()(k.o_orderpriority));
    }
};
}
void q4GroupBy(Table& tin, Table& origin, Table& tout) {
    std::unordered_map<q4GroupKey, int64_t> ht1;
    for (int i = 0; i < tin.getNumRow(); i++) {
        int32_t o_rowid = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> orderpriority =
            origin.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(o_rowid, 0);
        // if(i<10) std::cout<<std::dec<<o_rowid<<" "<<orderpriority.data()<<std::endl;
        q4GroupKey k{std::string(orderpriority.data())};
        auto it = ht1.find(k);
        if (it != ht1.end()) {
            int64_t s = it->second + 1;
            ht1[k] = s;
        } else {
            ht1.insert(std::make_pair(k, 1));
        }
    }
    int r = 0;
    for (auto& it : ht1) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> orderpriority{};
        memcpy(orderpriority.data(), (it.first.o_orderpriority).data(), (it.first.o_orderpriority).length());
        tout.setcharN<char, TPCH_READ_MAXAGG_LEN + 1>(r, 0, orderpriority);
        tout.setInt64(r, 1, it.second);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q4GroupBy" << std::endl;
}

void q4Sort(Table& tin, Table& tout) {
    struct Q4SortRow {
        std::string o_orderpriority;
        int64_t count;
    };

    struct {
        // operator <
        bool operator()(const Q4SortRow& a, const Q4SortRow& b) const { return a.o_orderpriority < b.o_orderpriority; }
    } Q4SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q4SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> orderpriority = tin.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(i, 0);
        Q4SortRow t = {std::string(orderpriority.data()), tin.getInt64(i, 1)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q4SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> orderpriority{};
        memcpy(orderpriority.data(), (it.o_orderpriority).data(), (it.o_orderpriority).length());
        tout.setcharN<char, TPCH_READ_MAXAGG_LEN + 1>(r, 0, orderpriority);
        tout.setInt64(r, 1, it.count);
        if (r < 10)
            std::cout << std::dec << " " << tout.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(r, 0).data() << " "
                      << tout.getInt64(r, 1) << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " In q4Sort" << std::endl;
}
