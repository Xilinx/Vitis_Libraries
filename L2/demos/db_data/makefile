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
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%L2/demos/*}')
XFLIB_DIR := $(abspath $(XF_PROJ_ROOT))

SF ?= 1

SSBDIR = $(XFLIB_DIR)/ext/ssb_dbgen
DATDIR = dat$(SF)

.PHONY: all run clean

all: $(DATDIR)/.stamp

%.exe: %.cpp
	$(CXX) $< -o $@ -I../host -std=c++11 -g -O3 -pthread

$(SSBDIR)/sf$(SF)/%.tbl:
	make -C $(SSBDIR) SF=$(SF)

$(DATDIR)/.stamp: columngen/columngen.exe | $(foreach f,lineitem orders region nation customer supplier part partsupp,$(SSBDIR)/sf$(SF)/$(f).tbl)
	mkdir -p $(DATDIR)
	./$< -in $(SSBDIR)/sf$(SF) -out $(DATDIR)
	touch $(DATDIR)/.stamp

clean:
	rm -f columngen/*.exe $(DATDIR)/*.dat $(DATDIR)/.stamp
