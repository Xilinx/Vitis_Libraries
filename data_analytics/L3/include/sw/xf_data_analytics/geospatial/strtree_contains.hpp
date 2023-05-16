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

#ifndef __XF_STRTREE_CONTAINS_HPP__
#define __XF_STRTREE_CONTAINS_HPP__

#include <vector>
#include <string>
#include <iostream>

namespace xf {
namespace data_analytics {
namespace geospatial {

/**
 * @brief strtree_contains geosptial contains (point in polygon) based on strtree
 *
 * @param mode 0: no fpga, 1: index function is accelerated by fpga
 * @param x_col the column number of x in point csv file
 * @param y_col the column number of y in point csv file
 * @param point_file input point csv file
 * @param polygon_file input polygon file
 * @param results The id of the first dimension represents the polygon id, and the content represents the point id
 * @param zone [xmin, xmax, ymin, ymax], Limit the scope of point and polygon.
 * @return total size of results
 *
 */
int strtree_contains(int mode,
                     int x_col,
                     int y_col,
                     std::string point_file,
                     std::string polygon_file,
                     double zone[4],
                     std::vector<std::vector<int> >& results);

/**
 * @brief copy2array convert vector to numpy.ndarray
 *
 * @param vecs input vector data
 * @param rangevec output pointer
 * @param n output size
 */
void copy2array(std::vector<std::vector<int> >& vecs, int* rangevec, int n);
//
}
}
}
#endif
