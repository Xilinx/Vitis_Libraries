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

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L1/demos/*}')
XFLIB_DIR := $(abspath $(XF_PROJ_ROOT))

SSBDIR = $(XFLIB_DIR)/ext/ssb_dbgen
DATDIR = dat

.PHONY: all exe run clean

all: $(DATDIR)/.stamp

exe: columngen/columnize.exe

%.exe: %.cpp
	$(CXX) $< -o $@ -I../host -std=c++11 -g -O3 -pthread

$(SSBDIR)/sf1/%.tbl:
	make -C $(SSBDIR)

$(DATDIR)/.stamp: columngen/columnize.exe | $(foreach f,lineitem, $(SSBDIR)/sf1/$(f).tbl)
	mkdir -p $(DATDIR)
	./$< -l $(SSBDIR)/sf1/lineitem.tbl  -d $(DATDIR)
	touch $(DATDIR)/.stamp

clean:
	rm -f columngen/*.exe $(DATDIR)/*.dat $(DATDIR)/.stamp
