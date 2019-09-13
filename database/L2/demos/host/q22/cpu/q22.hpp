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
// only for host
bool c_phone_int_in(int32_t c_phone_int) {
    if (c_phone_int == (('1' << 8) + ('3')) || c_phone_int == (('3' << 8) + ('1')) ||
        c_phone_int == (('2' << 8) + ('3')) || c_phone_int == (('2' << 8) + ('9')) ||
        c_phone_int == (('3' << 8) + ('0')) || c_phone_int == (('1' << 8) + ('8')) ||
        c_phone_int == (('1' << 8) + ('7'))) {
        return true;
    }
    return false;
}
// customer->t1
void Substring(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    for (int i = 0; i < nrow; i++) {
        int32_t c_custkey = tin.getInt32(i, 0);
        std::array<char, TPCH_READ_PHONE_LEN + 1> c_phone = tin.getcharN<char, TPCH_READ_PHONE_LEN + 1>(i, 1);
        int32_t c_acctbal = tin.getInt32(i, 2);
        int c_phone_int = 0;
        for (int j = 1; j >= 0; j--) {
            int subint = c_phone.at(1 - j) << (8 * j);
            c_phone_int += subint;
        }
        tout.setInt32(i, 0, c_custkey);
        tout.setInt32(i, 1, c_phone_int);
        tout.setInt32(i, 2, c_acctbal);
    }
    tout.setNumRow(nrow);
    std::cout << std::dec << nrow << " after Substring of Customer" << std::endl;
}
// t1 avg
bool iswrong(int32_t c) {
    if (c == 1741 || c == 34434 || c == 36078 || c == 146548) {
        return false;
    }
    return true;
}
int32_t Group_AVG(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int64_t sum = 0;
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        int32_t c_phone_int = tin.getInt32(i, 1);
        int32_t c_acctbal = tin.getInt32(i, 2);
        // if(c==1741||c==34434||c==36078||c==146548){
        //    std::cout<<c_acctbal<<std::endl;
        // }
        if (c_acctbal > 0 && c_phone_int_in(c_phone_int)) { //&&iswrong(c)){
            sum += c_acctbal;
            r++;
        }
    }
    std::cout << std::dec << "count:" << r << " avg is: " << 100 * sum / r << std::endl;
    return sum / r;
}

// t1->t2
void customerFilter_c_acctbal(Table& tin, Table& tout, int32_t avg_acct) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        int32_t c_custkey = tin.getInt32(i, 0);
        int32_t c_phone_int = tin.getInt32(i, 1);
        int32_t c_acctbal = tin.getInt32(i, 2);
        if (c_acctbal > avg_acct) {
            tout.setInt32(r, 0, c_custkey);
            tout.setInt32(r, 1, c_phone_int);
            tout.setInt32(r, 2, c_acctbal);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after customerFilter_c_acctbal" << std::endl;
}
// t2 orders antijoin->t3
void q22AntiJoin_o_t2(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    int nrow2 = tin2.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        int32_t o_custkey = tin1.getInt32(i, 0);
        ht1.insert(std::make_pair(o_custkey, 0));
    }
    int r = 0;
    for (int i = 0; i < nrow2; i++) {
        int32_t c_phone_int = tin2.getInt32(i, 1);
        int32_t c_custkey = tin2.getInt32(i, 0);
        int32_t c_acctbal = tin2.getInt32(i, 2);
        auto it = ht1.find(c_custkey);
        if (c_phone_int_in(c_phone_int) && it == ht1.end()) {
            tout.setInt32(r, 0, c_phone_int);
            tout.setInt32(r, 1, c_acctbal);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " after q22AntiJoin_t1_t2" << std::endl;
}
void q22GroupBy(Table& tin, Table& tout) {
    struct q22Groupload {
        int32_t numcust;
        int64_t sumacct;
    };
    std::unordered_map<int32_t, q22Groupload> m;
    unsigned nrow = tin.getNumRow();
    for (size_t i = 0; i < nrow; ++i) {
        int32_t c_phone_int = tin.getInt32(i, 0);
        int32_t c_acctbal = tin.getInt32(i, 1);
        auto it = m.find(c_phone_int);
        if (it != m.end()) {
            int32_t s0 = it->second.numcust + 1;
            int64_t s1 = it->second.sumacct + c_acctbal;
            m[c_phone_int].numcust = s0; // update
            m[c_phone_int].sumacct = s1; // update
        } else {
            m.insert(std::make_pair(c_phone_int, q22Groupload{1, c_acctbal}));
        }
    }
    int r = 0;
    for (auto& it : m) {
        tout.setInt32(r, 0, it.first);
        tout.setInt32(r, 1, it.second.numcust);
        tout.setInt64(r, 2, it.second.sumacct);
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q22GroupBy" << std::endl;
}

void q22Sort(Table& tin, Table& tout) {
    struct Q22SortRow {
        int32_t cntrycode;
        int32_t numcust;
        int64_t sumacct;
    };
    struct {
        // operator <
        bool operator()(const Q22SortRow& a, const Q22SortRow& b) const { return a.cntrycode < b.cntrycode; }
    } Q22SortLess;

    int nrow = tin.getNumRow();
    std::vector<Q22SortRow> rows;
    for (int i = 0; i < nrow; i++) {
        int32_t cntrycode = tin.getInt32(i, 0);
        int32_t numcust = tin.getInt32(i, 1);
        int64_t sumacct = tin.getInt64(i, 2);
        Q22SortRow t = {cntrycode, numcust, sumacct};
        rows.push_back(t);
    }

    std::sort(rows.begin(), rows.end(), Q22SortLess);

    int r = 0;
    for (auto& it : rows) {
        tout.setInt32(r, 0, it.cntrycode);
        tout.setInt32(r, 1, it.numcust);
        tout.setInt64(r, 2, it.sumacct);
        if (r < 10)
            std::cout << std::dec << tout.getInt32(r, 0) << " " << tout.getInt32(r, 1) << " " << tout.getInt64(r, 2)
                      << std::endl;
        ++r;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " After q22Sort" << std::endl;
}
