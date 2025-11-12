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
#
import os
import sys
import argparse
import numpy as np
PATH = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(PATH, "../common/scripts"))

import random
import gen_matrix_utils as utils

mapper = {
    "float": np.float64,
    "cfloat": np.complex128
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("LOC_INPUT_FILE", type=str)
    parser.add_argument("DATA_TYPE", type=str)
    parser.add_argument("DIM_SIZE", type=int)
    parser.add_argument("NUM_MATRICES", type=int)
    parser.add_argument("SEED", type=int, default=42, nargs='?')
    args = parser.parse_args()

    LOC_INPUT_FILE = args.LOC_INPUT_FILE
    DATA_TYPE = args.DATA_TYPE
    DIM_SIZE = args.DIM_SIZE
    NUM_MATRICES = args.NUM_MATRICES
    SEED = args.SEED

    os.makedirs(os.path.dirname(LOC_INPUT_FILE), exist_ok=True)
    random.seed(SEED)

    line_to_file = ""

    for i in range(NUM_MATRICES):
        if DATA_TYPE == "float":
            matrix = utils.generate_real_matrix(DIM_SIZE, DIM_SIZE)
        elif DATA_TYPE == "cfloat":
            matrix = utils.generate_complex_matrix(DIM_SIZE, DIM_SIZE)
        else:
            raise TypeError("Only float and cfloat supported.")

        matrix_hermetian = np.array(utils.get_hermetian(matrix), dtype=mapper[DATA_TYPE])
        eigenvalues, eigenvectors = np.linalg.eigh(matrix_hermetian)

        desired_condition_number = 10
        eigenvalue_min_threshold = max(eigenvalues) / desired_condition_number
        eigenvalues_adjusted = np.maximum(eigenvalues, eigenvalue_min_threshold)

        matrix_hermetian_well_conditioned = eigenvectors @ np.diag(eigenvalues_adjusted) @ eigenvectors.conj().T

        matrix_serial = utils.get_serial(matrix_hermetian_well_conditioned, row_major=False)
        
        with open(LOC_INPUT_FILE, "a") as file:
            for num in matrix_serial:
                file.write(str(num)+"\n")
