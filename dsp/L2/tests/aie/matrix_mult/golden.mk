#
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
#

###############################################################################
# Makefile helper used for MM compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../


HELPER:= $(HELPER_CUR_DIR)/.helper

NUM_INPUT_SAMPLES = $(shell echo $$((P_DIM_A * P_DIM_B)))
NUM_MACS_PER_KERNEL = $(shell echo $$(( ( $(P_DIM_A) * $(P_DIM_AB) * $(P_DIM_B) ) / $(P_CASC_LEN) )))
P_INPUT_WINDOW_VSIZE_A = $(shell echo $$(( $(P_DIM_A) * $(P_DIM_AB))))
P_INPUT_WINDOW_VSIZE_B = $(shell echo $$(( $(P_DIM_B) * $(P_DIM_AB))))
PARAM_MAP = T_DATA_A $(T_DATA_A) T_DATA_B $(T_DATA_B) P_DIM_A $(P_DIM_A) P_DIM_AB $(P_DIM_AB) P_DIM_B $(P_DIM_B) P_SHIFT $(P_SHIFT) P_ROUND_MODE $(P_ROUND_MODE) P_SAT_MODE $(P_SAT_MODE) P_DIM_A_LEADING $(P_DIM_A_LEADING) P_DIM_B_LEADING $(P_DIM_B_LEADING) P_DIM_OUT_LEADING $(P_DIM_OUT_LEADING) P_ADD_TILING_A $(P_ADD_TILING_A) P_ADD_TILING_B $(P_ADD_TILING_B) P_ADD_DETILING_OUT $(P_ADD_DETILING_OUT) P_INPUT_WINDOW_VSIZE_A $(P_INPUT_WINDOW_VSIZE_A) P_INPUT_WINDOW_VSIZE_B $(P_INPUT_WINDOW_VSIZE_B) P_CASC_LEN $(P_CASC_LEN)
UUT_FILE_SUFFIX = $(UUT_KERNEL)_$(T_DATA_A)_$(T_DATA_B)_$(STIM_TYPE_A)_$(STIM_TYPE_B)_$(P_SHIFT)_$(P_ROUND_MODE)_$(P_SAT_MODE)_$(P_DIM_A)_$(P_DIM_AB)_$(P_DIM_B)_$(P_INPUT_WINDOW_VSIZE_A)_$(P_INPUT_WINDOW_VSIZE_B)_$(P_ADD_TILING_A)_$(P_ADD_TILING_B)_$(P_ADD_DETILING_OUT)_$(P_DIM_A_LEADING)_$(P_DIM_B_LEADING)_$(P_DIM_OUT_LEADING)
LOG_FILE =./logs/log.txt
STATUS_LOG_FILE = ./logs/status.txt
STATUS_FILE = $(STATUS_LOG_FILE)

$(HELPER):
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(P_INPUT_WINDOW_VSIZE_A) $(NITER) 1 $(STIM_TYPE_A) 0 0 $(T_DATA_A)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(P_INPUT_WINDOW_VSIZE_B) $(NITER) 2 $(STIM_TYPE_B) 0 0 $(T_DATA_B)
	TARGET=x86sim UUT_KERNEL=matrix_mult_ref UUT_SIM_FILE=./data/ref_output.txt make run TARGET=x86sim TAG=REF
	make cleanall
	echo perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --inFile $(LOC_INPUT_FILE_A) --inRow $(P_DIM_A)  --inCol $(P_DIM_AB) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --cascLen $(P_CASC_LEN) --colMajor $(P_DIM_A_LEADING) --isTiled $(P_ADD_TILING_A) --tileInPlace
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --inFile $(LOC_INPUT_FILE_A) --inRow $(P_DIM_A)  --inCol $(P_DIM_AB) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --cascLen $(P_CASC_LEN) --colMajor $(P_DIM_A_LEADING) --isTiled $(P_ADD_TILING_A) --tileInPlace 
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --inFile $(LOC_INPUT_FILE_B) --inRow $(P_DIM_AB) --inCol $(P_DIM_B) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --cascLen $(P_CASC_LEN) --colMajor $(P_DIM_B_LEADING) --isTiled $(P_ADD_TILING_B)  --tileInPlace  --splitRows

mm_partition_shuffle:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/matrix_mult_partition_shuffle.pl --untile --inFile ./logs/uut_output.txt --tileInPlace --inRow $(P_DIM_A) --inCol $(P_DIM_B) --T_DATA_A $(T_DATA_A) --T_DATA_B $(T_DATA_B) --colMajor $(P_DIM_OUT_LEADING) --isTiled $(P_ADD_DETILING_OUT)

get_latency:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(STATUS_FILE) $(NUM_INPUT_SAMPLES) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(NUM_INPUT_SAMPLES) $(P_CASC_LEN) $(STATUS_FILE) ./aiesimulator_output matMult_impl1

get_status: check_op_ref
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_diff:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)

create_config:
	echo $(STATUS_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)