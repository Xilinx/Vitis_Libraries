#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
AIESIMFLAGS += --display-run-interval=1000000

#Adding extra parameters
ifneq (,$(findstring VCD,$(PARAMS)))
	AIESIMFLAGS += --dump-vcd ${UUT_KERNEL}_sim --simulation-cycle-timeout=100000
endif

# Parses "CLEAN_TEST" from the test json.
CLEAN_TEST := $(shell $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/parse_clean_test.py '$(PARAMS_FILE)' '$(PARAMS)')

# Parses "QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE" from the test json.
QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE := $(shell $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/parse_qor_helper_constraints_path.py '$(PARAMS_FILE)' '$(PARAMS)')

# Parses "PROMPT_FILE" from the test json (a file name under the IP's prompts/ directory).
# A prompt-driven test configures the IP from a natural-language prompt in pre_build, so it takes
# precedence over the QoR helper: the QoR helper recommendation and prediction check are disabled.
ifeq ($(origin PROMPT_FILE),undefined)
PROMPT_FILE := $(shell $(VITIS_PYTHON3) $(XFLIB_DIR)/L2/tests/aie/common/scripts/parse_test_field.py '$(PARAMS_FILE)' '$(PARAMS)' PROMPT_FILE)
endif
ifneq ($(strip $(PROMPT_FILE)),)
ifneq ($(strip $(QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE)),)
$(info INFO: PROMPT_FILE=$(PROMPT_FILE) takes precedence, ignoring QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE=$(QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE))
endif
override QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE :=
endif
