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

all: 
	make -C $(CUR_DIR) lib

oniguruma:
	git clone https://github.com/kkos/oniguruma.git oniguruma

lib: oniguruma
	cd oniguruma && sh ./autogen.sh
	cd oniguruma && ./configure --prefix=$(CUR_DIR)/lib CFLAGS="-O3 -gdwarf-2 -g3"
	cd oniguruma && make
	cd oniguruma && make install

clean:
	rm -fr $(DEST_DIR)/oniguruma $(DEST_DIR)/lib

