#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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

import numpy as np

def getSegmentNodeForPyramid(sideLength: int, numSegments: int, nodeIdx: int) -> float:
    """
    Function calculates the nodes at which to divide a triangular-based right-angled pyramid such that each divided 
    segment has equal volume. The shape of this pyramid is analogous to the shape of the Cholesky computation, where the 
    X-axis corresponds to the X-dimension of the matrix, Y-axis to the Y-dimension of the matrix, and Z-axis to the 
    stage in the computation. The region bounded by this pyramid corresponds to the elements being operated on.
    Ergo, in stage 0, the entire lower-triangular region of the matrix is being operated on, and in the final stage, 
    only the bottom-rightmost element is being operated on.

    The below diagram denotes a usage example with 3 segments. Calling with nodeIdx for 0, 1, 2, 3 will return the
    corresponding distances along the Z-axis. This function returns ideal splits to provide the exact equal volume,
    therefore distances are decimal.
      
             +--------------------+    ↑
            /|\                  /|    |
          /  |*\               /  |    |
        +--------------------+    |    |
        |   |**|##\          |    |    | 
        |  |**|###|\         |    |    | sideLength (Y-axis) 
        |  |**|###|%%\       |    |    | 
        | |**|###|%%%%\      |    |    |  
        | |**|###|%%%%%%\    |    |    |   
        ||**|###|%%%%%%%%\---|----+  ↑ ↓   
        ||**|###|%%%%%%%%%%\ |   /  /     
        |**|###|%%%%%%%%%%%%\| /  /  sideLength (X-axis)
        +==/===/=============+  ↓ 
        0  1   2             3 
        ←--------------------→ 
          sideLength (Z-axis)

    """
    return sideLength * (1 - (1 - nodeIdx / numSegments)**(1/3))

if __name__ == "__main__":
    
    TP_DIM_max = 1024
    TP_CASC_max = 32

    cascade_stages_table = np.zeros((TP_DIM_max, TP_CASC_max), dtype=int)

    for TP_DIM in range(1, TP_DIM_max+1):
        for TP_CASC_LEN in range(1, TP_CASC_max+1):
            numStagesFloat = getSegmentNodeForPyramid(TP_DIM, TP_CASC_LEN, 1)
            numStagesInt = int(round(numStagesFloat))
            if numStagesInt < 1:
                numStagesInt = 1

            cascade_stages_table[TP_DIM-1][TP_CASC_LEN-1] = numStagesInt

    np.savetxt("cascade_stages_table.csv", cascade_stages_table, delimiter=",", fmt="%d")