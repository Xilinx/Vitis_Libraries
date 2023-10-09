#
# Copyright 2021 Xilinx, Inc.
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


HELPER:= $(HELPER_CUR_DIR)/.helper

WINDOW_VSIZE_A 	= $(shell echo $$(( $(DIM_A) * $(DIM_B) * $(NUM_FRAMES))))
WINDOW_VSIZE_B 	= $(shell echo $$(( $(DIM_B) * $(NUM_FRAMES))))
PARAM_MAP = DATA_A $(DATA_A) DATA_B $(DATA_B) DIM_A $(DIM_A) DIM_B $(DIM_B) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE) NUM_FRAMES $(NUM_FRAMES) CASC_LEN $(CASC_LEN)
UUT_FILE_SUFFIX = $(UUT_KERNEL)_$(DATA_A)_$(DATA_B)_$(DIM_A)_$(DIM_B)_$(NUM_FRAMES)_$(SHIFT)_$(ROUND_MODE)_$(SAT_MODE)_$(WINDOW_VSIZE_A)_$(WINDOW_VSIZE_B)_$(CASC_LEN)_$(STIM_TYPE_A)_$(STIM_TYPE_B)
LOG_FILE =./logs/log.txt
STATUS_LOG_FILE = ./logs/status.txt
STATUS_FILE = $(STATUS_LOG_FILE)

$(HELPER):
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(WINDOW_VSIZE_A) $(NITER) 0 $(STIM_TYPE_A) 0 0 $(DATA_A) 0 1;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(WINDOW_VSIZE_B) $(NITER) 0 $(STIM_TYPE_B) 0 0 $(DATA_B) 0 1;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --inFile $(LOC_INPUT_FILE_A) --inRow $(DIM_A)  --inCol $(DIM_B) --T_DATA_A $(DATA_A) --T_DATA_B $(DATA_A) --cascLen $(CASC_LEN) --colMajor 1 --isTiled 1 --tileInPlace    |& tee -a $(LOG_FILE);\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --inFile $(LOC_INPUT_FILE_B) --inRow 1         --inCol $(DIM_B) --T_DATA_A $(DATA_B) --T_DATA_B $(DATA_B) --cascLen $(CASC_LEN) --colMajor 1 --isTiled 1 --tileInPlace    |& tee -a $(LOG_FILE);\
	TARGET=x86sim UUT_KERNEL=matrix_vector_mul_ref UUT_SIM_FILE=./data/ref_output.txt make run TARGET=x86sim TAG=REF
	make cleanall

get_latency:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(STATUS_FILE) $(WINDOW_VSIZE_A) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE_A) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output matVecMulMain $(NITER)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_diff:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)

get_theoretical_min:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_mtx_theoretical_min.tcl $(DATA_A) $(DATA_B) $(WINDOW_VSIZE_A) $(CASC_LEN) $(STATUS_FILE) $(UUT_KERNEL)

create_config:
	echo $(STATUS_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts