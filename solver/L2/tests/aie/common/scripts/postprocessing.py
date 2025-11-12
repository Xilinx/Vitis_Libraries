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
import argparse
import numpy as np

byte_widths_per_dtype = {
    "uint8": 1,
    "int8": 1,
    "uint16": 2,
    "int16": 2,
    "float16": 2,
    "uint32": 4,
    "int32": 4,
    "float": 4,
    "cbfloat16": 2, # complex widths are per scalar value.
    "cfloat": 4,
    "cint16": 2,
    "cint32": 4
}

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("INPUT_FILE_LOC", type=str)
    parser.add_argument("OUTPUT_FILE_LOC", type=str)
    parser.add_argument("DATA_TYPE", type=str)
    parser.add_argument("PLIO_WIDTH", type=int, default=64, nargs='?')
    args = parser.parse_args()

    INPUT_FILE_LOC = args.INPUT_FILE_LOC
    OUTPUT_FILE_LOC =args.OUTPUT_FILE_LOC
    DATA_TYPE = args.DATA_TYPE
    PLIO_WIDTH = args.PLIO_WIDTH // 8

    elems_per_line = PLIO_WIDTH // byte_widths_per_dtype[DATA_TYPE]
    in_data = np.loadtxt(INPUT_FILE_LOC, dtype=str)
    out_data = in_data.reshape((-1, elems_per_line))
    np.savetxt(OUTPUT_FILE_LOC, out_data, fmt='%s', delimiter=' ', newline=' \n')
