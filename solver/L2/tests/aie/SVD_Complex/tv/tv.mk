# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
# vitis makefile-generator v2.0.9
#

CXX = g++

HOST_SRCS += main.cpp

CXXFLAGS += -I./ -I../aie -g

LDFLAGS += -pthread

EXE_FILE = app.exe

$(EXE_FILE) : $(HOST_SRCS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

all: $(EXE_FILE)
run: all
	./$(EXE_FILE);
tv: run
	mkdir -p ../data
	mv A0.txt ../data
	mv A1.txt ../data
	mv Gld0.txt ../data
	mv Gld1.txt ../data
	rm -rf $(EXE_FILE)
clean:
	rm -rf $(EXE_FILE)
cleanall: clean
