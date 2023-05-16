#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import containsModule
import numpy as np 
from datetime import datetime

# GeoSpatial Contains (Point in Polygon) based on STRTree
class STRTree:
    # 0: no fpga, 1: index function is accelerated by fpga
    # x_col the id of x column in point_file
    # y_col the id of y column in point_file
    # point_file a csv file
    # polygon_file
    # zone [xmin, xmax, ymin, ymax]
    def __init__(self, mode, x_col, y_col, point_file, polygon_file, zone):
        self.mode = mode
        self.x_col = x_col
        self.y_col = y_col
        self.point_file = point_file
        self.polygon_file = polygon_file
        self.zone = containsModule.doubleArray(4)
        for i in range(4):
            self.zone[i] = zone[i]
    
    # execute contains, return a numpy.ndarray (polygon id, point id)
    def contains(self):
        results = containsModule.IntVector2(0)
        sz = containsModule.strtree_contains(self.mode, self.x_col, self.y_col, self.point_file, self.polygon_file, self.zone, results)
        arr = containsModule.copy2array(results, sz * 2);
        res = np.reshape(arr, (2, -1), order='F')
        return res

