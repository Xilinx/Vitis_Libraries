#!/bin/bash
#
# Copyright 2022 Xilinx, Inc.
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
dummy=$2
data_memory=0
target_fname="*mapping_analysis_report.txt"
grep Memory\ Bank\ Report $loc/reports/$target_fname -A 10000 | grep MG\( -A 10000 | tail -n +3 > temp.txt

tr -s \  < temp.txt > f.txt 
cut -d" " -f2,4 f.txt | sort -u > f1.txt
cat f1.txt | cut -d" " -f1|sort -u |tail -n +2 >sort_key.txt
while read -r line
do
   name="$line"
   srch_key=$name
   temp=$(grep $srch_key f1.txt | cut -d" " -f2 |sort -u | awk '{ sum+=$1} END {print sum}')
   arr=($temp)
   dm=${arr[${#arr[@]}-1]}
   data_memory=$(echo "$data_memory + $dm"|bc)
done < "sort_key.txt"

echo $data_memory
