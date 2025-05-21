#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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

###############################################################################
# Makefile helper used for MM compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) COARSE_BITS $(COARSE_BITS) FINE_BITS $(FINE_BITS) DOMAIN_MODE $(DOMAIN_MODE) WINDOW_VSIZE $(WINDOW_VSIZE) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE)
EXTRA_PARAM_MAP =  STIM_TYPE $(STIM_TYPE) DIFF_TOLERANCE $(DIFF_TOLERANCE) CC_TOLERANCE $(CC_TOLERANCE) NITER $(NITER) FUNC_CHOICE $(FUNC_CHOICE)
INPUT_WINDOW_VSIZE = $(shell echo $$(($(WINDOW_VSIZE)*$(NUM_INPUTS))))
DATA_SEED ?= 0
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
SPLIT_ZIP_FILE ?=

sim_ref:
	make run TARGET=x86sim TAG=REF ;\
	make cleanall

gen_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/func_approx/gen_input_approx.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DATA_TYPE) $(COARSE_BITS) $(FINE_BITS) $(DOMAIN_MODE) 64

diff:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) $(EXTRA_PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input.txt ./data/uut_output.txt $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output funcApprox $(NITER)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
