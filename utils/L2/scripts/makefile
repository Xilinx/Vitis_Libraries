#
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
#

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MK_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
SHELL := /bin/bash
override XFLIB_DIR := $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L2/scripts/*}')

DEST_DIR ?= $(CURDIR)

# Use override, in case the host is cross-compiled with CXX set.
ifneq ($(shell expr $(shell g++ -dumpversion) \>= 5), 1)
ifndef XILINX_VIVADO
CXX_ERROR = [ERROR]: g++ version 5.0 or above is required.
else
override CXX = $(XILINX_VIVADO)/tps/lnx64/gcc-6.2.0/bin/g++
CXX_WARNING = [WARNING]: g++ before 5.0 is not supported. Using g++ provided by Vivado: $(CXX)
endif
else
override CXX = g++
endif

export PIP_DISABLE_PIP_VERSION_CHECK=1

.PHONY: all check_cxx
all: $(DEST_DIR)/dataConverter $(DEST_DIR)/py3env/.ok

check_cxx:
ifneq (,$(CXX_ERROR))
	@echo $(CXX_ERROR) >&2 && false
else ifneq (,$(CXX_WARNING))
	@echo $(CXX_WARNING) >&2
endif

$(DEST_DIR)/dataConverter: $(XFLIB_DIR)/L2/src/sw/datamover/data_converter.cpp check_cxx
	@echo "[INFO] Compiling text data file converter..."
	@mkdir -p $(DEST_DIR)
	@$(CXX) -o $@ -I $(XILINX_VITIS)/include -I $(XILINX_HLS)/include -I $(XFLIB_DIR)/L1/include $< -std=c++14

$(DEST_DIR)/py3env/.ok: $(DEST_DIR)/py3env/bin/activate
	@echo "[INFO] Installing required python3 packages in venv..."
	@(source $(DEST_DIR)/py3env/bin/activate; PYTHONPATH="" python3 $(DEST_DIR)/py3env/bin/pip install -r $(MK_DIR)/requirements.txt)
	@touch $(DEST_DIR)/py3env/.ok

$(DEST_DIR)/py3env/bin/activate:
	@echo "[INFO] Creating python3 virtual environment..."
	@mkdir -p $(DEST_DIR)
	@(cd $(DEST_DIR); python3 -m venv py3env)

clean:
	@rm -rf dataConverter py3env
