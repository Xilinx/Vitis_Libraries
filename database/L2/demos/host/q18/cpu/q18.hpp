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
// t1:1500000
void q18GroupBy(Table& tin, Table& tout) {
    std::unordered_map<int32_t, int64_t> m;
    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int32_t l_orderkey = tin.getInt32(i, 0); // index much patMatch YAML
        int32_t l_quantity = tin.getInt32(i, 1); // index much patMatch YAML
        auto it = m.find(l_orderkey);
        if (it != m.end()) {
            int64_t s = it->second + l_quantity;
            m[l_orderkey] = s; // update
            if (l_orderkey == 1) std::cout << std::dec << s << std::endl;
        } else {
            m.insert(std::make_pair(l_orderkey, l_quantity));
        }
    }
    int r = 0;
    for (auto& it : m) {
        tout.setInt32(r, 0, it.first);
        tout.setInt64_l(r, 1, it.second);
        tout.setInt64_h(r, 2, it.second);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q18GroupBy" << std::endl;
}
// t1 order
void q18Join_t1_o(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int64_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t l_orderkey = tin1.getInt32(i, 0);
        int32_t sum_l = tin1.getInt32(i, 1);
        int32_t sum_h = tin1.getInt32(i, 2);
        int64_t sum = tin1.mergeInt64(sum_l, sum_h);
        if (sum > 300) {
            ht1.insert(std::make_pair(l_orderkey, sum));
        }
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t o_orderkey = tin2.getInt32(i, 0);
        auto it = ht1.find(o_orderkey);
        if (it != ht1.end()) {
            int32_t o_custkey = tin2.getInt32(i, 1);
            int32_t o_orderdate = tin2.getInt32(i, 2);
            int32_t o_totalprice = tin2.getInt32(i, 3);
            tout.setInt32(r, 0, o_orderkey);
            tout.setInt32(r, 1, o_custkey);
            tout.setInt32(r, 2, o_orderdate);
            tout.setInt32(r, 3, o_totalprice);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q18Join_t1_o" << std::endl;
}
// c_name c_rowid
void q18Join_t2_c(Table& tin1, Table& tin2, Table& tout) {
    struct KeyA {
        int32_t o_orderkey;
        int32_t o_orderdate;
        int32_t o_totalprice;
    };
    int nrow1 = tin1.getNumRow();
    std::unordered_multimap<int32_t, KeyA> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t o_custkey = tin1.getInt32(i, 1);
        int32_t o_orderdate = tin1.getInt32(i, 2);
        int32_t o_totalprice = tin1.getInt32(i, 3);
        ht1.insert(std::make_pair(o_custkey, KeyA{o_orderkey, o_orderdate, o_totalprice}));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t c_custkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(c_custkey);
        auto it = its.first;
        while (it != its.second) {
            int32_t c_rowid = i; // tin2.getInt32(i,2);
            int32_t o_orderkey = it->second.o_orderkey;
            int32_t o_orderdate = it->second.o_orderdate;
            int32_t o_totalprice = it->second.o_totalprice;
            tout.setInt32(r, 0, o_orderkey);
            tout.setInt32(r, 1, o_orderdate);
            tout.setInt32(r, 2, o_totalprice);
            tout.setInt32(r, 3, c_rowid);
            tout.setInt32(r, 4, c_custkey);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q18Join_t2_c" << std::endl;
}
void q18Join_t3_l(Table& tin1, Table& tin2, Table& tout) {
    struct KeyA {
        int32_t o_orderdate;
        int32_t o_totalprice;
        int32_t c_rowid;
        int32_t c_custkey;
    };
    int nrow1 = tin1.getNumRow();
    std::unordered_multimap<int32_t, KeyA> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_orderkey = tin1.getInt32(i, 0);
        int32_t o_orderdate = tin1.getInt32(i, 1);
        int32_t o_totalprice = tin1.getInt32(i, 2);
        int32_t c_rowid = tin1.getInt32(i, 3);
        int32_t c_custkey = tin1.getInt32(i, 4);
        ht1.insert(std::make_pair(o_orderkey, KeyA{o_orderdate, o_totalprice, c_rowid, c_custkey}));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t l_orderkey = tin2.getInt32(i, 0);
        auto its = ht1.equal_range(l_orderkey);
        auto it = its.first;
        if (it != its.second) {
            int32_t l_quantity = tin2.getInt32(i, 1);
            int32_t o_orderdate = it->second.o_orderdate;
            int32_t o_totalprice = it->second.o_totalprice;
            int32_t c_rowid = it->second.c_rowid;
            int32_t c_custkey = it->second.c_custkey;
            tout.setInt32(r, 0, l_orderkey);
            tout.setInt32(r, 1, o_orderdate);
            tout.setInt32(r, 2, o_totalprice);
            tout.setInt32(r, 3, c_rowid);
            tout.setInt32(r, 4, c_custkey);
            tout.setInt32(r, 5, l_quantity);
            it++;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q18Join_t3_l" << std::endl;
}

struct Q18GroupKey {
    int32_t o_orderkey;
    int32_t o_orderdate;
    int32_t o_totalprice;
    std::string c_name;
    int32_t c_custkey;
    bool operator==(const Q18GroupKey& other) const {
        return (c_name == other.c_name) && (c_custkey == other.c_custkey) && (o_orderkey == other.o_orderkey) &&
               (o_orderdate == other.o_orderdate) && (o_totalprice == other.o_totalprice);
    }
};
namespace std {

template <>
struct hash<Q18GroupKey> {
    std::size_t operator()(const Q18GroupKey& k) const {
        using std::size_t;
        using std::hash;
        using std::string;
        return (hash<string>()(k.c_name)) + (hash<int>()(k.c_custkey)) + (hash<int>()(k.o_orderkey)) +
               (hash<int>()(k.o_orderdate)) + (hash<int>()(k.o_totalprice));
    }
};
}

void q18GroupBy(Table& tin, Table& origintb, Table& tout) {
    std::cout << "DEBUG" << std::endl;
    std::unordered_map<Q18GroupKey, int64_t> m;
    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int32_t o_orderkey = tin.getInt32(i, 0);
        int32_t o_orderdate = tin.getInt32(i, 1);
        int32_t o_totalprice = tin.getInt32(i, 2);
        int32_t c_rowid = tin.getInt32(i, 3);
        std::array<char, TPCH_READ_C_NAME_LEN + 1> c_name_array =
            origintb.getcharN<char, TPCH_READ_C_NAME_LEN + 1>(c_rowid, 1);
        std::string c_name = std::string(c_name_array.data());
        int32_t c_custkey = tin.getInt32(i, 4);
        int32_t l_quantity = tin.getInt32(i, 5);
        Q18GroupKey k{o_orderkey, o_orderdate, o_totalprice, c_name, c_custkey};
        auto it = m.find(k);
        if (it != m.end()) {
            int64_t s = it->second + l_quantity;
            m[k] = s; // update
        } else {
            m.insert(std::make_pair(k, l_quantity));
        }
    }
    int r = 0;
    for (auto& it : m) {
        tout.setInt32(r, 0, it.first.o_orderkey);
        tout.setInt32(r, 1, it.first.o_orderdate);
        tout.setInt32(r, 2, it.first.o_totalprice);
        std::array<char, TPCH_READ_C_NAME_LEN + 1> c_name_array{};
        memcpy(c_name_array.data(), (it.first.c_name).data(), (it.first.c_name).length());
        tout.setcharN<char, TPCH_READ_C_NAME_LEN + 1>(r, 3, c_name_array);
        tout.setInt32(r, 4, it.first.c_custkey);
        tout.setInt64(r, 5, it.second);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q18GroupBy" << std::endl;
}

void q18Sort(Table& tin, Table& tout) {
    struct Q18SortRow {
        int32_t o_orderkey;
        int32_t o_orderdate;
        int32_t o_totalprice;
        std::string c_name;
        int32_t c_custkey;
        int64_t sum;
    };

    struct {
        // operator <
        bool operator()(const Q18SortRow& a, const Q18SortRow& b) const {
            return a.o_totalprice > b.o_totalprice ||
                   (a.o_totalprice == b.o_totalprice && a.o_orderdate < b.o_orderdate);
        }
    } Q18SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q18SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        int32_t o_orderkey = tin.getInt32(i, 0);
        int32_t o_orderdate = tin.getInt32(i, 1);
        int32_t o_totalprice = tin.getInt32(i, 2);
        std::array<char, TPCH_READ_C_NAME_LEN + 1> c_name_array = tin.getcharN<char, TPCH_READ_C_NAME_LEN + 1>(i, 3);
        int32_t c_custkey = tin.getInt32(i, 4);
        int64_t sum = tin.getInt64(i, 5);
        Q18SortRow t = {o_orderkey, o_orderdate, o_totalprice, std::string(c_name_array.data()), c_custkey, sum};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q18SortLess);

    int r = 0;
    for (auto& it : rows) {
        std::array<char, TPCH_READ_C_NAME_LEN + 1> c_name{};
        memcpy(c_name.data(), (it.c_name).data(), (it.c_name).length());
        tout.setcharN<char, TPCH_READ_C_NAME_LEN + 1>(r, 0, c_name);
        tout.setInt32(r, 1, it.c_custkey);
        tout.setInt32(r, 2, it.o_orderkey);
        tout.setInt32(r, 3, it.o_orderdate);
        tout.setInt32(r, 4, it.o_totalprice);
        tout.setInt64(r, 5, it.sum);
        if (r < 10)
            std::cout << std::dec << c_name.data() << " " << it.c_custkey << " " << it.o_orderkey << " "
                      << it.o_orderdate << " " << it.o_totalprice << " " << it.sum << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q18Sort" << std::endl;
}
