#!/bin/bash
#
# Copyright (C) 2024-2025, Advanced Micro Devices, Inc.
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

# Assume the first command-line argument is the location of the reports directory
loc=$1

# Define the path to the complexity.csv file
complexity_file="$loc/reports/complexity.csv"

# Read the total_num_cores from complexity.csv and store it in a variable
total_num_cores=$(grep total_num_cores "$complexity_file" | cut -d, -f2)

# Echo the TOTAL_NUM_CORES value to the terminal
echo $total_num_cores