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
// l_returnflag, l_linestatus, l_quantity  l_extendedprice l_discount l_tax l_shipdate
void q1FilterL(Table& tin, Table& tout) {
    int r = 0;
    int nrow = tin.getNumRow();
    for (int i = 0; i < nrow; ++i) {
        int32_t l_shipdate = tin.getInt32(i, 6);
        // if(i<10) std::cout<<std::dec<<l_shipdate<<std::endl;
        if (l_shipdate <= 19980902) {
            int32_t l_returnflag = tin.getInt32(i, 0);
            int32_t l_linestatus = tin.getInt32(i, 1);
            int32_t l_quantity = tin.getInt32(i, 2);
            int32_t l_extendedprice = tin.getInt32(i, 3);
            int32_t l_discount = tin.getInt32(i, 4);
            int32_t l_tax = tin.getInt32(i, 5);
            tout.setInt32(r, 0, l_returnflag);
            tout.setInt32(r, 1, l_linestatus);
            tout.setInt32(r, 2, l_quantity);
            tout.setInt32(r, 3, l_extendedprice);
            tout.setInt32(r, 4, l_discount);
            tout.setInt32(r, 5, l_tax);
            tout.setInt32(r, 6, l_shipdate);
            ++r;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " in q1FilterL" << std::endl;
}

struct Q1GroupKey {
    int32_t k1; // l_returnflag
    int32_t k2; // l_linestatus

    bool operator==(const Q1GroupKey& other) const { return (k1 == other.k1 && k2 == other.k2); }
};
namespace std {

template <>
struct hash<Q1GroupKey> {
    std::size_t operator()(const Q1GroupKey& k) const {
        using std::size_t;
        using std::hash;
        // using std::string;
        return (hash<int>()(k.k1)) + (hash<int>()(k.k2));
    }
};
}
struct Q1GroupValue {
    int64_t sum_qty;        // sum(l_quantity)
    int64_t sum_price;      // sum(l_extendedprice)
    int64_t sum_disc_price; // sum(l_extendedprice * (1 - l_discount))
    int64_t sum_charge;     // sum(l_extendedprice * (1 - l_discount) * (1 + l_tax))
    // avg(l_quantity)      sum_qty/sum_count
    // avg(l_extendedprice) sum_price/sum_count
    int64_t sum_disc;  // avg(l_discount) sum_disc/sum_count
    int64_t sum_count; // count(*)
};

void q1GroupBy(Table& tin, Table& tout) {
    std::unordered_map<Q1GroupKey, Q1GroupValue> m;

    int nrow = tin.getNumRow();
    for (int i = 0; i < nrow; ++i) {
        int32_t l_returnflag = tin.getInt32(i, 0);
        int32_t l_linestatus = tin.getInt32(i, 1);
        int32_t l_quantity = tin.getInt32(i, 2); // index much patMatch YAML
        int32_t l_extendedprice = tin.getInt32(i, 3);
        int32_t l_discount = tin.getInt32(i, 4);
        int32_t l_tax = tin.getInt32(i, 5);

        int32_t eval0 = l_extendedprice * (100 - l_discount) / 100;
        int64_t eval1 = (int64_t)l_extendedprice * (100 - l_discount) * (100 + l_tax) / 10000;
        Q1GroupKey k{l_returnflag, l_linestatus};
        auto it = m.find(k);
        if (it != m.end()) {
            int64_t sum_qty = it->second.sum_qty + l_quantity;
            int64_t sum_price = it->second.sum_price + l_extendedprice;
            int64_t sum_disc_price = it->second.sum_disc_price + eval0;
            int64_t sum_charge = it->second.sum_charge + eval1;
            int64_t sum_disc = it->second.sum_disc + l_discount;
            int64_t sum_count = it->second.sum_count + 1;
            Q1GroupValue s{sum_qty, sum_price, sum_disc_price, sum_charge, sum_disc, sum_count};
            m[k] = s;
        } else {
            Q1GroupValue s{l_quantity, l_extendedprice, eval0, eval1, l_discount, 1};
            m.insert(std::make_pair(k, s));
        }
    }

    int r = 0;
    for (auto& it : m) {
        int64_t sum_qty = it.second.sum_qty;
        int64_t sum_price = it.second.sum_price;
        int64_t sum_disc = it.second.sum_disc;
        int64_t sum_count = it.second.sum_count;
        int64_t avg_qty = sum_qty / sum_count;
        int64_t avg_price = sum_price / sum_count;
        int64_t avg_disc = sum_disc / sum_count;

        tout.setInt32(r, 0, it.first.k1);
        tout.setInt32(r, 1, it.first.k2);
        tout.setInt64(r, 2, sum_qty);
        tout.setInt64(r, 3, sum_price);
        tout.setInt64(r, 4, it.second.sum_disc_price);
        tout.setInt64(r, 5, it.second.sum_charge);
        tout.setInt64(r, 6, avg_qty);
        tout.setInt64(r, 7, avg_price);
        tout.setInt64(r, 8, avg_disc);
        tout.setInt64(r, 9, sum_count);
        ++r;
    }
    tout.setNumRow(r);
}
void q1Sort(Table& tin, Table& tout) {
    struct Q1SortRow {
        int32_t l_returnflag;
        int32_t l_linestatus;
        int64_t sum_qty;
        int64_t sum_price;
        int64_t sum_disc_price;
        int64_t sum_charge;
        int64_t avg_qty;
        int64_t avg_price;
        int64_t avg_disc;
        int64_t sum_count;
    };

    struct {
        // operator <
        bool operator()(const Q1SortRow& a, const Q1SortRow& b) const {
            return a.l_returnflag < b.l_returnflag ||
                   (a.l_returnflag == b.l_returnflag && a.l_linestatus < b.l_linestatus);
        }
    } Q1SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q1SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        Q1SortRow t = {tin.getInt32(i, 0), tin.getInt32(i, 1), tin.getInt64(i, 2), tin.getInt64(i, 3),
                       tin.getInt64(i, 4), tin.getInt64(i, 5), tin.getInt64(i, 6), tin.getInt64(i, 7),
                       tin.getInt64(i, 8), tin.getInt64(i, 9)};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q1SortLess);

    int r = 0;
    for (auto& it : rows) {
        tout.setInt32(r, 0, it.l_returnflag);
        tout.setInt32(r, 1, it.l_linestatus);
        tout.setInt64(r, 2, it.sum_qty);
        tout.setInt64(r, 3, it.sum_price);
        tout.setInt64(r, 4, it.sum_disc_price);
        tout.setInt64(r, 5, it.sum_charge);
        tout.setInt64(r, 6, it.avg_qty);
        tout.setInt64(r, 7, it.avg_price);
        tout.setInt64(r, 8, it.avg_disc);
        tout.setInt64(r, 9, it.sum_count);
        if (r < 10)
            std::cout << std::dec << tout.getInt32(r, 0) << " " << tout.getInt32(r, 1) << " " << tout.getInt64(r, 2)
                      << " " << tout.getInt64(r, 3) << " " << tout.getInt64(r, 4) << " " << tout.getInt64(r, 5) << " "
                      << tout.getInt64(r, 6) << " " << tout.getInt64(r, 7) << " " << tout.getInt64(r, 8) << " "
                      << tout.getInt64(r, 9) << std::endl;
        ++r;
    }
    tout.setNumRow(r);
}
void q1Print(Table& tout) {
    int nrow = tout.getNumRow();
    printf("after sort:%d\n", nrow);
    int r = nrow > 32 ? 32 : nrow;
    std::cout << "Query result:" << nrow << std::endl;
    for (int i = 0; i < r; ++i) {
        int32_t l_returnflag = tout.getInt32(r, 0);
        int32_t l_linestatus = tout.getInt32(r, 1);
        int64_t sum_qty = tout.getInt64(r, 2);
        int64_t sum_price = tout.getInt64(r, 3);
        int64_t sum_disc_price = tout.getInt64(r, 4);
        int64_t sum_charge = tout.getInt64(r, 5);
        int64_t avg_qty = tout.getInt64(r, 6);
        int64_t avg_price = tout.getInt64(r, 7);
        int64_t avg_disc = tout.getInt64(r, 8);
        int64_t sum_count = tout.getInt64(r, 9);
        printf(
            "l_returnflag:%d, "
            "l_linestatus:%d,sum_qty:%ld,sum_price:%ld,sum_disc_price:%ld,sum_charge:%ld,avg_qty:%ld,avg_price:%ld,avg_"
            "disc:%ld,sum_count:%ld\n",
            l_returnflag, l_linestatus, sum_qty, sum_price, sum_disc_price, sum_charge, avg_qty, avg_price, avg_disc,
            sum_count);
    }
}
