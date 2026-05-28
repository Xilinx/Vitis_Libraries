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
import sys
import argparse

def main():
    """
    Interlaces blocks of lines from multiple input files into a single output file,
    accounting for the number of samples per line based on data type.
    """
    parser = argparse.ArgumentParser(description="Simple sample-based interlacing of text files.")
    parser.add_argument("block_size_samples", type=int, help="Number of samples in each atomic block to interlace.")
    parser.add_argument("data_type", type=str, choices=['float', 'cfloat'], help="Data type of the samples.")
    parser.add_argument("num_files", type=int, help="Number of input files to interlace.")
    parser.add_argument("num_iterations", type=int, help="Number of iterations (matrices) in each file.")
    parser.add_argument("input_prefix", type=str, help="Prefix for input files, e.g., 'data/uut_output'.")
    parser.add_argument("output_file", type=str, help="Path to the output file.")
    args = parser.parse_args()

    if args.data_type == 'cfloat':
        samples_per_line = 1
    else: # float
        samples_per_line = 2

    if args.block_size_samples % samples_per_line != 0:
        print(f"Warning: Block size in samples ({args.block_size_samples}) is not divisible by samples per line ({samples_per_line}).", file=sys.stderr)
    
    lines_per_block = args.block_size_samples // samples_per_line

    input_filenames = [f"{args.input_prefix}_{i}_0.txt" for i in range(args.num_files)]
    
    try:
        input_file_handles = [open(name, 'r') for name in input_filenames]
    except FileNotFoundError as e:
        print(f"Error opening input files: {e}", file=sys.stderr)
        sys.exit(1)

    with open(args.output_file, 'w') as out_f:
        for _ in range(args.num_iterations):
            for in_f in input_file_handles:
                for _ in range(lines_per_block):
                    line = in_f.readline()
                    if line:
                        out_f.write(line)
                    else:
                        break
    
    for f in input_file_handles:
        f.close()

if __name__ == "__main__":
    main()
