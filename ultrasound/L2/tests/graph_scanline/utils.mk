# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#+-------------------------------------------------------------------------------
# The following parameters are assigned with default values. These parameters can
# be overridden through the make command line
#+-------------------------------------------------------------------------------

#Check vitis setup
ifndef XILINX_VITIS
  XILINX_VITIS = /opt/xilinx/Vitis/$(TOOL_VERSION)
  export XILINX_VITIS
endif

.PHONY: check_device
check_device:
	@set -eu; \
	inallowlist=False; \
	inblocklist=False; \
	for dev in $(PLATFORM_ALLOWLIST); \
	    do if [[ $$(echo $(PLATFORM_NAME) | grep $$dev) != "" ]]; \
		then inallowlist=True; fi; \
	done ;\
	for dev in $(PLATFORM_BLOCKLIST); \
	    do if [[ $$(echo $(PLATFORM_NAME) | grep $$dev) != "" ]]; \
		then inblocklist=True; fi; \
	done ;\
	if [[ $$inallowlist == False ]]; \
	    then echo "[Warning]: The device $(PLATFORM_NAME) not in allowlist."; \
	fi; \
	if [[ $$inblocklist == True ]]; \
	    then echo "[ERROR]: The device $(PLATFORM_NAME) in blocklist."; exit 1;\
	fi;

ifneq ($(XPART),)
XPLATFORM := $(XPART)
PLATFORM_NAME = $(XPART)
else
ifneq (,$(wildcard $(PLATFORM)))
# Use PLATFORM as a file path
XPLATFORM := $(PLATFORM)
else
# Use PLATFORM as a file name pattern
# 1. search paths specified by variable
ifneq (,$(PLATFORM_REPO_PATHS))
# 1.1 as exact name
XPLATFORM := $(strip $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/$(PLATFORM)/$(PLATFORM).xpfm)))
# 1.2 as a pattern
ifeq (,$(XPLATFORM))
XPLATFORMS := $(foreach p, $(subst :, ,$(PLATFORM_REPO_PATHS)), $(wildcard $(p)/*/*.xpfm))
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(PLATFORM)/')))
endif # 1.2
endif # 1
# 2. search Vitis installation
ifeq (,$(XPLATFORM))
# 2.1 as exact name vitis < 2022.2
XPLATFORM := $(strip $(wildcard $(XILINX_VITIS)/platforms/$(PLATFORM)/$(PLATFORM).xpfm))
ifeq (,$(XPLATFORM))
# 2.2 as exact name vitis >= 2022.2
XPLATFORM := $(strip $(wildcard $(XILINX_VITIS)/base_platforms/$(PLATFORM)/$(PLATFORM).xpfm))
# 2.3 as a pattern vitis < 2022.2
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard $(XILINX_VITIS)/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(PLATFORM)/')))
# 2.4 as a pattern vitis >= 2022.2
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard $(XILINX_VITIS)/base_platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(PLATFORM)/')))
endif # 2.4
endif # 2.3
endif # 2.2
endif # 2
# 3. search default locations
ifeq (,$(XPLATFORM))
# 3.1 as exact name
XPLATFORM := $(strip $(wildcard /opt/xilinx/platforms/$(PLATFORM)/$(PLATFORM).xpfm))
# 3.2 as a pattern
ifeq (,$(XPLATFORM))
XPLATFORMS := $(wildcard /opt/xilinx/platforms/*/*.xpfm)
XPLATFORM := $(strip $(foreach p, $(XPLATFORMS), $(shell echo $(p) | awk '$$1 ~ /$(PLATFORM)/')))
endif # 3.2
endif # 3
endif
XPLATFORM := $(firstword $(XPLATFORM))
#Get PLATFORM_NAME by PLATFORM
PLATFORM_NAME = $(strip $(patsubst %.xpfm, % , $(shell basename $(XPLATFORM))))
endif

define MSG_PLATFORM
No platform matched pattern '$(PLATFORM)'.
Available platforms are: $(XPLATFORMS)
To add more platform directories, set the PLATFORM_REPO_PATHS variable or point PLATFORM variable to the full path of platform .xpfm file.
endef
export MSG_PLATFORM

.PHONY: check_platform
check_platform:
ifeq (,$(XPLATFORM))
	@echo "$${MSG_PLATFORM}" && false
endif

#Check ends

.PHONY: check_version
check_version:
ifneq (, $(shell which git))
ifneq (,$(wildcard $(XFLIB_DIR)/.git))
	@cd $(XFLIB_DIR) && git log --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit -n 1 && cd -
endif
endif

#Setting VPP
VPP := v++

#Cheks for aiecompiler
AIECXX := aiecompiler
AIESIMULATOR := aiesimulator
X86SIMULATOR := x86simulator

.PHONY: check_vpp
check_vpp:
ifeq (,$(wildcard $(XILINX_VITIS)/bin/v++))
	@echo "Cannot locate Vitis installation. Please set XILINX_VITIS variable." && false
endif

export PATH := $(XILINX_VITIS)/bin:$(XILINX_XRT)/bin:$(PATH)

# Cleaning stuff
RM = rm -f
RMDIR = rm -rf

MV = mv -f
CP = cp -rf
ECHO:= @echo
PYTHON3 ?= python3
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(XILINX_VITIS)/tps/lnx64/python-3.8.3/lib $(XILINX_VITIS)/tps/lnx64/python-3.8.3/bin/python3
