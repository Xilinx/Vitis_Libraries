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
short=$2
num_me_cores=0
target_fname="*mapping_analysis_report.txt"

if [ $short -eq 0 ]
then
     rowcol_list=(0 1 2 3 4 5 6 7)
else
     rowcol_list=(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49)
fi

for row in ${rowcol_list[*]}
do
   for col in ${rowcol_list[*]}
   do
     #temp=$(grep CR\(x\,y $loc/reports/$target_fname -A 10000 | grep this | grep CR\($row,$col\) -c)
     temp=$(grep Port\ Mapping\ Report $loc/reports/$target_fname -B 10000 | grep CR\($row,$col\) -c)
     if [ $temp -gt 0  ]
     then
         temp=1
     else
         temp=0
     fi
     num_me_cores=$(echo "$num_me_cores + $temp"|bc)
   done
done
echo $num_me_cores
