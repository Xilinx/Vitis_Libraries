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
void q3FilterC(Table& tin, Table& tout) {
    int r = 0;
    int nrow = tin.getNumRow();
    for (int i = 0; i < nrow; ++i) {
        int32_t c_custkey = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_MAXAGG_LEN + 1> c_mktsegment = tin.getcharN<char, TPCH_READ_MAXAGG_LEN + 1>(i, 1);
        if (!strcmp(c_mktsegment.data(), "BUILDING")) {
            tout.setInt32(r, 0, c_custkey);
            ++r;
        }
    }
    tout.setNumRow(r);
}

struct Q3GroupKey {
    int32_t k1;
    int32_t k2;
    int32_t k3;

    bool operator==(const Q3GroupKey& other) const { return (k1 == other.k1 && k2 == other.k2 && k3 == other.k3); }
};
namespace std {

template <>
struct hash<Q3GroupKey> {
    std::size_t operator()(const Q3GroupKey& k) const {
        using std::size_t;
        using std::hash;
        // using std::string;
        return (hash<int>()(k.k1)) + (hash<int>()(k.k2)) + (hash<int>()(k.k3));
    }
};
}
void q3GroupBy(Table& tin, Table& tout) {
    std::unordered_map<Q3GroupKey, int64_t> m;

    unsigned nrow = tin.getNumRow();
    for (unsigned int i = 0; i < nrow; ++i) {
        int32_t l_orderkey = tin.getInt32(i, 0); // index much patMatch YAML
        int32_t o_orderdate = tin.getInt32(i, 1);
        int32_t o_shippriority = tin.getInt32(i, 2);
        int32_t eval0 = tin.getInt32(i, 3);
        int32_t r = eval0;
        Q3GroupKey k{l_orderkey, o_orderdate, o_shippriority};
        auto it = m.find(k);
        if (it != m.end()) {
            int64_t s = it->second + r;
            m[k] = s; // update
        } else {
            m.insert(std::make_pair(k, r));
        }
    }

    int r = 0;
    for (auto& it : m) {
        tout.setInt32(r, 0, it.first.k1);
        tout.setInt32(r, 1, it.first.k2);
        tout.setInt32(r, 2, it.first.k3);
        tout.setInt64(r, 3, it.second);
        ++r;
    }
    tout.setNumRow(r);
}
void q3Sort(Table& tin, Table& tout) {
    struct Q3SortRow {
        int32_t l_orderkey;
        int32_t o_orderdate;
        int32_t o_shipdate;
        int64_t revenue;
    };

    struct {
        // operator <
        bool operator()(const Q3SortRow& a, const Q3SortRow& b) const {
            return a.revenue > b.revenue || (a.revenue == b.revenue && a.o_orderdate < b.o_orderdate);
        }
    } Q3SortLess;

    int nrow = tin.getNumRow();
    std::cout << nrow << " before sort!" << std::endl;
    std::vector<Q3SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        Q3SortRow t = {tin.getInt32(i, 15), tin.getInt32(i, 14), tin.getInt32(i, 13), tin.getUint32(i, 0)};
        rows.push_back(t);
        // std::cout<<t.l_orderkey<<" "<<t.o_orderdate<<" "<<t.o_shipdate<<" "<<t.revenue<<std::endl;
    }

    std::sort(rows.begin(), rows.end(), Q3SortLess);

    int r = 0;
    for (auto& it : rows) {
        tout.setInt32(r, 0, it.l_orderkey);
        tout.setInt32(r, 1, it.o_orderdate);
        tout.setInt32(r, 2, it.o_shipdate);
        tout.setInt64(r, 3, it.revenue);
        ++r;
    }
    tout.setNumRow(r);
}

void q3Print(Table& tin) {
    unsigned nrow = tin.getNumRow();
    printf("after sort:%d\n", nrow);
    int r = nrow > 32 ? 32 : nrow;
    std::cout << "Query result:" << nrow << std::endl;
    for (int i = 0; i < r; ++i) {
        int32_t l_orderkey = tin.getInt32(i, 0); // index much patMatch YAML
        int32_t o_orderdate = tin.getInt32(i, 1);
        int32_t o_shippriority = tin.getInt32(i, 2);
        int64_t eval0 = tin.getInt64(i, 3);
        printf("l_orderkey:%d, o_orderdate:%d,o_shippriority:%d,eval0:%ld\n", l_orderkey, o_orderdate, o_shippriority,
               eval0);
    }
}
void q3Join_C_O1(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    std::cout << std::dec << ht1.size() << "  q3Join_C_O1" << std::endl;
    struct KeyC {
        int32_t o_orderkey;
        int32_t o_custkey;
        int32_t o_orderdate;
        int32_t o_shippriority;
    };
    std::vector<KeyC> ht2;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; ++i) {
        int o_orderdate = tin2.getInt32(i, 2);
        if (o_orderdate < 19950315) {
            int o_orderkey = tin2.getInt32(i, 0);
            int o_custkey = tin2.getInt32(i, 1);
            int o_shippriority = tin2.getInt32(i, 3);
            ht2.push_back(KeyC{o_orderkey, o_custkey, o_orderdate, o_shippriority});
        }
    }
    // std::cout<<std::dec<<ht2.size()<<"  q3Join_C_O1"<<std::endl;
    int r = 0;
    for (unsigned int i = 0; i < ht2.size(); i++) {
        int o_custkey = ht2[i].o_custkey;
        auto it = ht1.find(o_custkey);
        if (it != ht1.end()) {
            int o_orderdate = ht2[i].o_orderdate;
            int o_shippriority = ht2[i].o_shippriority;
            int o_orderkey = ht2[i].o_orderkey;
            tout.setInt32(r, 0, o_orderdate);
            tout.setInt32(r, 1, o_shippriority);
            tout.setInt32(r, 2, o_orderkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q3Join_C_O1" << std::endl;
}

void q3Join_C_O2(Table& tin1, Table& tin2, Table& tout) {
    struct KeyA {
        int32_t o_orderdate;
        int32_t o_shippriority;
    };
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, KeyA> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 2), KeyA{tin1.getInt32(i, 0), tin1.getInt32(i, 1)}));
    }
    struct KeyC {
        int32_t l_orderkey;
        int32_t l_extendedprice;
        int32_t l_discount;
        int32_t l_shipdate;
    };
    std::vector<KeyC> ht2;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; ++i) {
        int l_shipdate = tin2.getInt32(i, 3);
        if (l_shipdate > 19950315) {
            int l_orderkey = tin2.getInt32(i, 0);
            int l_extendedprice = tin2.getInt32(i, 1);
            int l_discount = tin2.getInt32(i, 2);
            ht2.push_back(KeyC{l_orderkey, l_extendedprice, l_discount, l_shipdate});
        }
    }
    std::cout << std::dec << ht2.size() << " q3Join_C_O2" << std::endl;
    int r = 0;
    for (unsigned int i = 0; i < ht2.size(); i++) {
        int l_orderkey = ht2[i].l_orderkey;
        auto it = ht1.find(l_orderkey);
        if (it != ht1.end()) {
            int o_orderdate = it->second.o_orderdate;
            int o_shippriority = it->second.o_shippriority;
            int32_t l_extendedprice = ht2[i].l_extendedprice;
            int32_t l_discount = ht2[i].l_discount;
            int eval0 = (-l_discount + 100) * l_extendedprice;
            tout.setInt32(r, 0, l_orderkey);
            tout.setInt32(r, 1, o_orderdate);
            tout.setInt32(r, 2, o_shippriority);
            tout.setInt32(r, 3, eval0);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " out q3Join_C_O2" << std::endl;
}
