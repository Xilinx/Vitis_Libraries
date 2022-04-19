/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _STRTREE_KERNEL_HPP_
#define _STRTREE_KERNEL_HPP_

#include "vpp_acc.hpp"
#include "xf_data_analytics/geospatial/strtree_wrap.hpp"
// NC node capacity
// #define NC 16

// key type
#define KT ap_uint<64>
// value type
#define VT ap_uint<32>
// point type
#define PT ap_uint<128>
// node (rectangle) type
#define NT ap_uint<256>

// 64 * 4 * 4 * 4 * 4
// 64 * 4 * 16 * 8 * 8 * 8
// 1024 * 4 * 64 * 16 * 16 * 16
// insert sort length
#define ISN (64)
// block (insert + merge) sort length
#define BSN (ISN * 4 * 4)
// merge tree channel number (ddr to ddr)
#define MTCN (4)
// max sort length on chip
#define MSN (BSN * MTCN * MTCN)

class strtree_acc : public VPP_ACC<strtree_acc, 1> {
    // port bindings
    ZERO_COPY(inX);
    ZERO_COPY(inY);
    ZERO_COPY(inZone);
    ZERO_COPY(extPointBuf0);
    ZERO_COPY(extPointBuf1);
    ZERO_COPY(extNodeBuf0);
    ZERO_COPY(extNodeBuf1);
    ZERO_COPY(extNodeBuf2);

    SYS_PORT(inX, DDR[0]);
    SYS_PORT(inY, DDR[0]);
    SYS_PORT(inZone, DDR[0]);
    SYS_PORT(extPointBuf0, DDR[1]);
    SYS_PORT(extPointBuf1, DDR[1]);
    SYS_PORT(extNodeBuf0, DDR[2]);
    SYS_PORT(extNodeBuf1, DDR[2]);
    SYS_PORT(extNodeBuf2, DDR[2]);

   public:
    static void compute(int sz,
                        double* inX,
                        double* inY,
                        double* inZone,
                        PT* extPointBuf0,
                        PT* extPointBuf1,
                        NT* extNodeBuf0,
                        NT* extNodeBuf1,
                        NT* extNodeBuf2);

    /**
     * @brief STRTree_Kernel strtree (a geospatial index) kernel
     * @param sz real size of inX or inY
     * @param inX all x value
     * @param inX all y value
     * @param inZone points (x, y) limit zone
     */
    static void STRTree_Kernel(int sz,
                               double* inX,
                               double* inY,
                               double* inZone,
                               PT* extPointBuf0,
                               PT* extPointBuf1,
                               NT* extNodeBuf0,
                               NT* extNodeBuf1,
                               NT* extNodeBuf2);
};
#endif
