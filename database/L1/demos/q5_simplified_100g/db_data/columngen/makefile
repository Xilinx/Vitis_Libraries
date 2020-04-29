#
# Copyright 2019 Xilinx, Inc.
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

all: columnize.exe

DEST = ../dat

%.exe: %.cpp
	xcpp -o $@ $< -I../../include -I../../host -std=c++11 -g -O3 -pthread

.PHONY: run clean

run: columnize.exe ../lineitem.tbl ../orders.tbl
	mkdir -p $(DEST)
	./$< -l ../lineitem.tbl -o ../orders.tbl -d $(DEST)

clean:
	rm -f *.exe *.dat *.txt
