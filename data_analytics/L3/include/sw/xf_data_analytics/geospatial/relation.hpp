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

#ifndef __XF_RELATION_HPP__
#define __XF_RELATION_HPP__

#include "common.hpp"

namespace xf {
namespace data_analytics {
namespace geospatial {

bool straddle(Line line1, Line line2) {
    int64_t res1 = (int64_t)(line1.x1 - line1.x2) * (int64_t)(line2.y1 - line1.y2) -
                   (int64_t)(line1.y1 - line1.y2) * (int64_t)(line2.x1 - line1.x2);
    int64_t res2 = (int64_t)(line1.x1 - line1.x2) * (int64_t)(line2.y2 - line1.y2) -
                   (int64_t)(line1.y1 - line1.y2) * (int64_t)(line2.x2 - line1.x2);
    if ((res1 <= 0 && res2 >= 0) || (res1 >= 0 && res2 <= 0))
        return true;
    else
        return false;
}

bool line_line_intersect(Line line1, Line line2) {
    return straddle(line1, line2) && straddle(line2, line1);
}

bool line_polygon_intersect(std::vector<std::pair<uint32_t, uint32_t> > polygon, Line line) {
    for (int i = 0; i < polygon.size() - 1; i++) {
        Line line_tmp = {polygon[i].first, polygon[i].second, polygon[i + 1].first, polygon[i + 1].second};
        if (line_line_intersect(line, line_tmp)) return true;
    }
    return false;
}

bool rectangle_polygon_intersect(std::vector<std::pair<uint32_t, uint32_t> > polygon, Node node) {
    std::vector<Line> lines;
    Line line1 = {node.xmin, node.ymin, node.xmax, node.ymin};
    Line line2 = {node.xmax, node.ymin, node.xmax, node.ymax};
    Line line3 = {node.xmax, node.ymax, node.xmin, node.ymax};
    Line line4 = {node.xmin, node.ymax, node.xmin, node.ymin};
    lines.push_back(line1);
    lines.push_back(line2);
    lines.push_back(line3);
    lines.push_back(line4);

    for (int i = 0; i < 4; i++) {
        if (line_polygon_intersect(polygon, lines[i])) return true;
    }
    return false;
}

bool point_in_rectangle(std::pair<uint32_t, uint32_t> point, Node rect) {
    if (point.first < rect.xmin || point.first > rect.xmax) return false;
    if (point.second < rect.ymin || point.second > rect.ymax) return false;
    return true;
}

bool point_in_polygon(std::pair<uint32_t, uint32_t> point, std::vector<std::pair<uint32_t, uint32_t> > polygon) {
    int cn = 0;
    // loop through all edges of the polygon
    for (int i = 0; i < polygon.size() - 1; i++) { // edge (polygon[i], polygon[i+1])
        if (((polygon[i].second <= point.second) && (polygon[i + 1].second > point.second)) ||
            ((polygon[i].second > point.second) && (polygon[i + 1].second <= point.second))) {
            // compute the actual edge-ray intersect x-coordinate
            double vt = ((double)((int32_t)(point.second - polygon[i].second))) /
                        (int32_t)(polygon[i + 1].second - polygon[i].second);
            double dx = vt * (int32_t)(polygon[i + 1].first - polygon[i].first);
            if (point.first < polygon[i].first + dx) // point.x < intersect
                ++cn;
        }
    }
    return (cn & 1);
}

bool point_in_polygon(Point point, std::vector<std::pair<uint32_t, uint32_t> > polygon) {
    int cn = 0;
    // loop through all edges of the polygon
    for (int i = 0; i < polygon.size() - 1; i++) { // edge (polygon[i], polygon[i+1])
        if (((polygon[i].second <= point.y) && (polygon[i + 1].second > point.y)) ||
            ((polygon[i].second > point.y) && (polygon[i + 1].second <= point.y))) {
            // compute the actual edge-ray intersect x-coordinate
            double vt = ((double)(point.y - polygon[i].second)) / (int32_t)(polygon[i + 1].second - polygon[i].second);
            double dx = vt * (int32_t)(polygon[i + 1].first - polygon[i].first);
            if (point.x < polygon[i].first + dx) // point.x < intersect
                ++cn;
        }
    }
    return (cn & 0x1);
}
} // geospatial
} // data_analytics
} // xf
#endif // __XF__RELATION_HPP__
