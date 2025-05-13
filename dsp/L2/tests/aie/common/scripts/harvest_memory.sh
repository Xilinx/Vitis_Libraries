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

STATUS_FILE=$1
PATHTOSCRIPTS=$2
WORK_DIR=Work

echo -n "    NUM_BANKS: "                                       >> $STATUS_FILE;\
$PATHTOSCRIPTS/get_num_banks.sh $WORK_DIR dummy                 >> $STATUS_FILE;\
echo -n "    NUM_AIE: "                                         >> $STATUS_FILE;\
$PATHTOSCRIPTS/get_num_me.sh $WORK_DIR 1                        >> $STATUS_FILE;\
echo -n "    NUM_CORES: "                                       >> $STATUS_FILE;\
$PATHTOSCRIPTS/get_num_cores.sh $WORK_DIR                       >> $STATUS_FILE;\
echo -n "    NUM_TILES: "                                       >> $STATUS_FILE;\
$PATHTOSCRIPTS/get_num_tiles.sh $WORK_DIR                       >> $STATUS_FILE;\
echo -n "    DATA_MEMORY: "                                     >> $STATUS_FILE;\
$PATHTOSCRIPTS/get_data_memory.sh $WORK_DIR                     >> $STATUS_FILE;\
echo -n "    PROGRAM_MEMORY: "                                  >> $STATUS_FILE;\
# Get the isolated (\b) numbers (\d) before "  total" using perl-like regex
max_prgmem=`ls $WORK_DIR/aie/*_*/Release/*_*.map|xargs grep -A 10 "Section summary for memory 'PM':"| grep -Po "\b\d+\b(?=\s+Total)"`;\
echo $max_prgmem                                                >> $STATUS_FILE;\
