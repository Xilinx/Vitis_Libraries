#!/bin/bash
#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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

loc=$1
data_memory=0
target_fname="*mapping_analysis_report.txt"

# Remove unrelated sections. Leave Memory Group report (lines starting with MG).
grep Memory\ Bank\ Report $loc/reports/$target_fname -A 10000 | grep MG\( -A 10000 | tail -n +3 > temp.txt

# Squeeze repeated whitespaces
tr -s \  < temp.txt > f.txt

# Cut out unrelated fields. Leave MG location buffer name and size. Sort -u (unique) to get rid of bufs reported as both on input and output of a kernel.
cut -d" " -f1,2,4 f.txt | sort -u > f1.txt

# Leave only buffer sizes
cat f1.txt | cut -d" " -f3|sort |tail -n +2 >sort_key.txt
while read -r line
do
   # Add it all together.
   data_memory=$(echo "$data_memory + $line"|bc)
done < "sort_key.txt"

echo $data_memory
