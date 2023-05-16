/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef __XF_COMMON_HPP__
#define __XF_COMMON_HPP__

#include <fstream>
#include <iostream>
#include <algorithm>
#include "utils.hpp"
#include "ap_int.h"

namespace xf {
namespace data_analytics {
namespace geospatial {
typedef ap_uint<32> u32;

const uint64_t Q = 4294967295; // 2^32-1

struct Node {
    u32 level; // 31bit -> leaf flag, 15-8 -> size, 7-0 -> level, others reserved
    u32 xmin;
    u32 ymin;
    u32 xmax;
    u32 ymax;
    u32 addr; // sub-node address in buffer
};

struct Point {
    u32 x;
    u32 y;
    u32 id;
};

struct Line {
    u32 x1;
    u32 y1;
    u32 x2;
    u32 y2;
};

template <typename DT>
struct Rectangle {
    DT xmin;
    DT xmax;
    DT ymin;
    DT ymax;
};

bool xComp(Point p1, Point p2) {
    return (p1.x < p2.x);
}

bool yComp(Point p1, Point p2) {
    return (p1.y < p2.y);
}

bool xNodeComp(Node n1, Node n2) {
    return (n1.xmax + n1.xmin < n2.xmax + n2.xmin);
}

bool yNodeComp(Node n1, Node n2) {
    return (n1.ymax + n1.ymin < n2.ymax + n2.ymin);
}

} // geospatial
} // data_analytics
} // xf

#endif // __COMMON_HPP__
