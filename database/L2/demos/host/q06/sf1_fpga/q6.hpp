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

void q6(Table& tin1, Table& tout) {
    int nrow = tin1.getNumRow();
    long long sum = 0;
    int r = 0;
    for (int i = 0; i < nrow; i++) {
        int32_t l_extendedprice = tin1.getInt32(i, 0);
        int32_t l_discount = tin1.getInt32(i, 1);
        int32_t l_shipdate = tin1.getInt32(i, 2);
        int32_t l_quantity = tin1.getInt32(i, 3);
        if (l_shipdate >= 19940101 && l_shipdate < 19950101 && l_discount >= 5 && l_discount <= 7 && l_quantity < 24) {
            sum += (l_extendedprice * l_discount);
            r++;
        }
    }
    std::cout << std::dec << sum << " In q6" << std::endl;
    printf("%d\n", r);
}
