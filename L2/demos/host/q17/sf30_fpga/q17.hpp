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
// no use
void q17Join_t3_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), tin1.getInt32(i, 1)));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t l_partkey = tin2.getInt32(i, 0);
        int32_t l_quantity = tin2.getInt32(i, 1);
        auto it = ht1.find(l_partkey);
        if (it != ht1.end()) {
            tout.setInt32(r, 0, l_partkey);
            tout.setInt32(r, 1, l_quantity);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q17Join_t3_l" << std::endl;
}
// t2
void PartFilter(Table& tin, Table& tout) {
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        std::array<char, TPCH_READ_P_BRND_LEN + 1> p_brand = tin.getcharN<char, TPCH_READ_P_BRND_LEN + 1>(i, 1);
        std::array<char, TPCH_READ_P_CNTR_LEN + 1> p_container = tin.getcharN<char, TPCH_READ_P_CNTR_LEN + 1>(i, 2);
        if (!strcmp(p_container.data(), "MED BOX") && !strcmp(p_brand.data(), "Brand#23")) {
            int32_t p_partkey = tin.getInt32(i, 0);
            tout.setInt32(r, 0, p_partkey);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q17PartFiler" << std::endl;
}
// t3
void q17Join_t2_l(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), 0));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t l_partkey = tin2.getInt32(i, 0);
        auto it = ht1.find(l_partkey);
        if (it != ht1.end()) {
            int32_t l_quantity = tin2.getInt32(i, 1);
            int32_t l_extendedprice = tin2.getInt32(i, 2);
            tout.setInt32(r, 0, l_partkey);
            tout.setInt32(r, 1, l_quantity);
            tout.setInt32(r, 2, l_extendedprice);
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q17Join_t2_l" << std::endl;
}
// input:t3 -> t1
void q17GroupBy(Table& tin, Table& tout) {
    struct Values {
        int64_t sum;
        int64_t count;
    };

    int nrow = tin.getNumRow();
    std::unordered_map<int32_t, Values> ht1;
    for (int i = 0; i < nrow; i++) {
        int32_t l_partkey = tin.getInt32(i, 0);
        int32_t l_quantity = tin.getInt32(i, 1);
        auto it = ht1.find(l_partkey);
        if (it != ht1.end()) {
            int64_t sum_ = it->second.sum + l_quantity;
            int64_t count_ = it->second.count + 1;
            Values v{sum_, count_};
            ht1[l_partkey] = v;
        } else {
            Values v{l_quantity, 1};
            ht1.insert(std::make_pair(l_partkey, v));
        }
    }
    int r = 0;
    for (auto it : ht1) {
        int64_t sum = it.second.sum;
        int64_t count = it.second.count;
        int32_t l_partkey = it.first;
        int32_t avg = 200 * sum / count;
        //        float avg_f =(float)sum/(float)count*0.2;
        //        if(r<10)std::cout<<avg<<std::endl;
        tout.setInt32(r, 0, l_partkey);
        tout.setInt32(r, 1, avg);
        r++;
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q17GroupBy_firt" << std::endl;
}
void q17Join_t1_t3(Table& tin1, Table& tin2, Table& tout) {
    int nrow1 = tin1.getNumRow();
    std::unordered_map<int32_t, int32_t> ht1;
    for (int i = 0; i < nrow1; i++) {
        ht1.insert(std::make_pair(tin1.getInt32(i, 0), tin1.getInt32(i, 1)));
    }
    int r = 0;
    int nrow2 = tin2.getNumRow();
    for (int i = 0; i < nrow2; i++) {
        int32_t l_partkey = tin2.getInt32(i, 0);
        auto it = ht1.find(l_partkey);
        if (it != ht1.end()) {
            int32_t l_quantity = tin2.getInt32(i, 1);
            int32_t l_extendedprice = tin2.getInt32(i, 2);
            tout.setInt32(r, 0, l_partkey);
            tout.setInt32(r, 1, it->second);
            tout.setInt32(r, 2, l_quantity);
            tout.setInt32(r, 3, l_extendedprice);
            //       if(l_partkey==135991)std::cout<<l_partkey<<" "<<it->second<<" "<<l_extendedprice<<std::endl;
            r++;
        }
    }
    tout.setNumRow(r);
    std::cout << std::dec << r << " q17Join_t1_t3" << std::endl;
}

void q17GroupBy_l(Table& tin, Table& tout) {
    int64_t sum = 0;
    int nrow = tin.getNumRow();
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        int32_t avg_l_quantity = tin.getInt32(i, 1);
        int32_t l_quantity = tin.getInt32(i, 2);
        int32_t l_extendedprice = tin.getInt32(i, 3);
        // if((float)l_quantity <avg_l_quantity*0.2){
        if (1000 * l_quantity < avg_l_quantity) {
            sum += l_extendedprice;
            r++;
        }
    }
    std::cout << std::dec << sum / 7 << " SUMi, " << r << " NUM" << std::endl;
}
