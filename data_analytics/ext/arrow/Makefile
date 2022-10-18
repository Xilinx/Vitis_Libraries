#
# Copyright 2022 Xilinx, Inc.
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
PYONY: all

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
DEST_DIR ?= $(CUR_DIR)
COMPILER_PATH ?= /usr/bin

all: ${DEST_DIR}/arrow/cpp/release

${DEST_DIR}/arrow:
	git clone https://github.com/apache/arrow.git ${DEST_DIR}/arrow
	cd ${DEST_DIR}/arrow && git checkout release-5.0.0

${DEST_DIR}/arrow/cpp/release: ${DEST_DIR}/arrow
	mkdir -p ${DEST_DIR}/arrow/cpp/release
	cd ${DEST_DIR}/arrow/cpp/release && cmake .. -DCMAKE_CXX_COMPILER=${COMPILER_PATH}/g++ -DCMAKE_C_COMPILER=${COMPILER_PATH}/gcc -DCMAKE_BUILD_TYPE=release -DARROW_CSV:BOOL=ON
	cd ${DEST_DIR}/arrow/cpp/release && make -j 8

clean:
	rm -fr $(DEST_DIR)/arrow

