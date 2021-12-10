/*
 * Copyright 2020 Xilinx, Inc.
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

#include "hls_EncodeCoeffOrders.hpp"

void top_order_tokenize(ap_uint<32> used_orders,
                        hls::stream<ap_uint<32> >& orderStrm,
                        hls::stream<ap_uint<64> >& tokenStrm,
                        hls::stream<bool>& e_tokenStrm) {
    xf::codec::hls_EncodeCoeffOrders(used_orders, orderStrm, tokenStrm, e_tokenStrm);
}
