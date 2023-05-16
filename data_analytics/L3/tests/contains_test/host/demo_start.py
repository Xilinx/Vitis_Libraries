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

import os
import sys
from strtree_contains import STRTree
from datetime import datetime
import numpy as np

if __name__ == "__main__":
    t1 = datetime.now()
    print("STRTree Contains Flow...")
    point_file = sys.argv[1]
    polygon_file = sys.argv[2]
    print(point_file, polygon_file)
    
    zone = [-74.2610, -73.6979, 40.4765, 40.9180]; # nyc zone
    tree = STRTree(0, 5, 6, point_file, polygon_file, zone) # init tree
    res = tree.contains() # execute contains
    
    t2 = datetime.now()
    print(["Total execution time (s): ", (t2 - t1).total_seconds()])
    
    print(res.shape, type(res), res)
    # np.savetxt("golden.txt", res, delimiter=',', fmt='%u')
    golden = np.loadtxt(sys.argv[3], dtype='uint', delimiter=',')
    if (res==golden).all():
        print("[INFO] Case right.")
    else:
        print("[ERROR] Case fail.")
        exit(-1)
