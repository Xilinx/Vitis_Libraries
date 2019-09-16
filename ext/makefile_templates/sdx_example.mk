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

# -----------------------------------------------------------------------------
#                          project common settings

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
CASE_ROOT ?= $(CUR_DIR)

.SECONDEXPANSION:

# -----------------------------------------------------------------------------
#                            sdx common setup

include $(MK_COMMON_DIR)/sdx_help.mk

include $(MK_COMMON_DIR)/vivado.mk

include $(MK_COMMON_DIR)/sdx.mk

include $(MK_COMMON_DIR)/set_platform.mk

# -----------------------------------------------------------------------------
# BEGIN_XF_MK_USER_SECTION
# -----------------------------------------------------------------------------
# TODO:          data creation and other user targets

# a (typically hidden) file as stamp
DATA_STAMP := $(CUR_DIR)/db_data/dat/.stamp
$(DATA_STAMP):
	make -C $(CUR_DIR)/db_data

.PHONY: data
data: $(DATA_STAMP)

# -----------------------------------------------------------------------------
# TODO:                          kernel setup

CASE_ROOT ?= $(CUR_DIR)
KSRC_DIR = $(CUR_DIR)/kernel

XCLBIN_NAME := join_kernel
XFREQUENCY := 280

KERNELS := join_kernel

# must provide path
join_kernel_EXTRA_HDRS = $(SRC_DIR)/table_dt.h
join_kernel_VPP_CFLAGS += -I$(SRC_DIR)

join_kernel_EXTRA_HDRS += $(CASE_ROOT)/../include/hw/xf_database/aggregate.h
join_kernel_EXTRA_HDRS += $(CASE_ROOT)/../include/hw/xf_database/hash_join.h
join_kernel_EXTRA_HDRS += $(CASE_ROOT)/../include/hw/xf_database/scan_col.h

VPP_CFLAGS += -I$(CASE_ROOT)/../include/hw

ifneq (,$(shell echo $(XPLATFORM) | awk '/u280/'))
# U280
join_kernel_VPP_CFLAGS += \
		       --sp join_kernel_1.s_unit:DDR[0] \
		       --sp join_kernel_1.t_unit:DDR[0] \
		       --sp join_kernel_1.hj_begin_status:DDR[1] \
		       --sp join_kernel_1.hj_end_status:DDR[1] \
		       --sp join_kernel_1.j_res:DDR[1] \
		       --sp join_kernel_1.pu0_ht:HBM[0] \
		       --sp join_kernel_1.pu1_ht:HBM[4] \
		       --sp join_kernel_1.pu2_ht:HBM[8] \
		       --sp join_kernel_1.pu3_ht:HBM[12] \
		       --sp join_kernel_1.pu4_ht:HBM[16] \
		       --sp join_kernel_1.pu5_ht:HBM[20] \
		       --sp join_kernel_1.pu6_ht:HBM[24] \
		       --sp join_kernel_1.pu7_ht:HBM[28] \
		       --sp join_kernel_1.pu0_s:HBM[2] \
		       --sp join_kernel_1.pu1_s:HBM[6] \
		       --sp join_kernel_1.pu2_s:HBM[10] \
		       --sp join_kernel_1.pu3_s:HBM[14] \
		       --sp join_kernel_1.pu4_s:HBM[18] \
		       --sp join_kernel_1.pu5_s:HBM[22] \
		       --sp join_kernel_1.pu6_s:HBM[26] \
		       --sp join_kernel_1.pu7_s:HBM[30]
CXXFLAGS += -DUSE_HBM
else ifneq (,$(shell echo $(XPLATFORM) | awk '/u200/ || /u250/'))
# U200 and U250
join_kernel_VPP_CFLAGS += \
		       --sp join_kernel_1.unit:bank0 \
		       --sp join_kernel_1.hj_begin_status:bank0 \
		       --sp join_kernel_1.hj_end_status:bank0 \
		       --sp join_kernel_1.j_res:bank1 \
		       --sp join_kernel_1.pu0_ht:bank2 \
		       --sp join_kernel_1.pu1_ht:bank2 \
		       --sp join_kernel_1.pu2_ht:bank2 \
		       --sp join_kernel_1.pu3_ht:bank2 \
		       --sp join_kernel_1.pu4_ht:bank3 \
		       --sp join_kernel_1.pu5_ht:bank3 \
		       --sp join_kernel_1.pu6_ht:bank3 \
		       --sp join_kernel_1.pu7_ht:bank3 \
		       --sp join_kernel_1.pu0_s:bank2 \
		       --sp join_kernel_1.pu1_s:bank2 \
		       --sp join_kernel_1.pu2_s:bank2 \
		       --sp join_kernel_1.pu3_s:bank2 \
		       --sp join_kernel_1.pu4_s:bank3 \
		       --sp join_kernel_1.pu5_s:bank3 \
		       --sp join_kernel_1.pu6_s:bank3 \
		       --sp join_kernel_1.pu7_s:bank3
CXXFLAGS += -DUSE_DDR
else
$(error Unsupported platform $(XPLATFORM))
endif

# -----------------------------------------------------------------------------
# TODO:                           host setup

SRC_DIR = $(CUR_DIR)/host

EXE_NAME = test_join
HOST_ARGS = -xclbin $(XCLBIN_FILE) -in $(CUR_DIR)/db_data/dat

SRCS = test_join

# must provide path
test_join_EXTRA_HDRS += $(EXT_DIR)/xcl2/xcl2.hpp $(KSRC_DIR)/join_kernel.h $(SRC_DIR)/table_dt.h $(SRC_DIR)/utils.h
test_join_CXXFLAGS += -I $(EXT_DIR)/xcl2 -I $(KSRC_DIR)

CXXFLAGS += -D XDEVICE=$(XDEVICE) -g

# EXTRA_OBJS is cannot be compiled from SRC_DIR, user should provide the rule
EXTRA_OBJS += xcl2

EXT_DIR = $(CASE_ROOT)/../ext
xcl2_SRCS = $(EXT_DIR)/xcl2/xcl2.cpp
xcl2_HDRS = $(EXT_DIR)/xcl2/xcl2.hpp
xcl2_CXXFLAGS = -I $(EXT_DIR)/xcl2

# -----------------------------------------------------------------------------
# END_XF_MK_USER_SECTION
# -----------------------------------------------------------------------------

.PHONY: all
all: host xclbin

include $(MK_COMMON_DIR)/sdx_kernel_rules.mk

include $(MK_COMMON_DIR)/sdx_host_rules.mk

include $(MK_COMMON_DIR)/sdx_test_rules.mk

