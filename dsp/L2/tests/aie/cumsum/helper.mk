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
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
SEED ?= 1

NUM_PORTS := 1
DYN_PT_SIZE := 0
LOG_POINT_SIZE := 0
API_IO := 0
DIM_A ?= 16
DIM_B ?= 1
NUM_FRAMES ?= 1
MODE ?= 0
DATA_TYPE ?= int16
DATA_OUT_TYPE ?= $(DATA_TYPE)
ALIGN_FACTOR := 1
ifeq ($(AIE_VARIANT), 22)
    ALIGN_FACTOR := 2
endif
ifeq ($(MODE),2)
    ALIGN_FACTOR := $(shell echo $$(( $(ALIGN_FACTOR) *2	)))
endif
ifeq ($(DATA_TYPE), int16)
	RAW_SAMPLES_IN_VECT		  := 16
else ifeq ($(DATA_TYPE), cint16)
	RAW_SAMPLES_IN_VECT 	  := 8
else ifeq ($(DATA_TYPE), int32)
	RAW_SAMPLES_IN_VECT 	  := 8
else ifeq ($(DATA_TYPE), cint32)
	RAW_SAMPLES_IN_VECT 	  := 4
else ifeq ($(DATA_TYPE), float)
	RAW_SAMPLES_IN_VECT 	  := 8
else ifeq ($(DATA_TYPE), cfloat)
	RAW_SAMPLES_IN_VECT 	  := 4
else ifeq ($(DATA_TYPE), bfloat16)
	RAW_SAMPLES_IN_VECT 	  := 16
else ifeq ($(DATA_TYPE), cbfloat16)
	RAW_SAMPLES_IN_VECT 	  := 8
else
	RAW_SAMPLES_IN_VECT 	  := 8
endif
SAMPLES_IN_VECT := $(shell echo $$(( 	$(RAW_SAMPLES_IN_VECT) * $(ALIGN_FACTOR) 		)))

DIM_A_CEIL := $(shell echo $$((   (($(DIM_A) + $(SAMPLES_IN_VECT) - 1) / $(SAMPLES_IN_VECT)) * $(SAMPLES_IN_VECT) ))) 
WINDOW_VSIZE := $(shell echo $$(( 	$(DIM_A_CEIL) * $(DIM_B) * $(NUM_FRAMES)		)))

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DATA_OUT_TYPE $(DATA_OUT_TYPE) DIM_A $(DIM_A) DIM_B $(DIM_B) NUM_FRAMES $(NUM_FRAMES) MODE $(MODE) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
NITER ?= 1
STIM_TYPE ?= 0
LOG_PT_SIZE ?= 0
ROUND_MODE ?= 0
SAT_MODE ?= 0
PLIO_WIDTH ?= 64
HEADER_SIZE ?= 0
NUM_PORTS ?= 1
HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_input:
	@echo helper.mk stage:	create_input
	@echo NUM_PORTS $(NUM_PORTS)
	@echo INPUT_FILE $(INPUT_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_POINT_SIZE) $(DATA_TYPE) $(API_IO) 1 0 0 0 0 0 ${PLIO_WIDTH} ${HEADER_SIZE};\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_POINT_SIZE) $(DATA_TYPE) $(API_IO) 1 0 0 0 0 0 ${PLIO_WIDTH} ${HEADER_SIZE};\
    perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --split --dual 0 -k $(HEADER_SIZE) -w $(WINDOW_VSIZE) --plioWidth ${PLIO_WIDTH} ;\
	echo Input ready

sim_ref:
	@echo helper.mk stage:	sim_ref
	make UUT_KERNEL=cumsum_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@echo helper.mk stage:	prep_x86_out
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@echo helper.mk stage:	prep_aie_out
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

get_diff:
	@echo helper.mk stage:	get_diff
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --zip --dual 0 -k $(HEADER_SIZE) -w $(WINDOW_VSIZE) --plioWidth ${PLIO_WIDTH} ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --zip --dual 0 -k $(HEADER_SIZE) -w $(WINDOW_VSIZE) --plioWidth ${PLIO_WIDTH} ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

get_status:
	@echo helper.mk stage:	get_status
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)

get_stats:
	@echo helper.mk stage:	get_stats
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output "cumsum_main" $(NITER)

harvest_mem:
	@echo helper.mk stage:	harvest_mem
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
