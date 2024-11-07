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
# Makefile helper used for outer_tensor compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
SEED ?= 0

DIM_OUT := $(shell echo $$(( $(DIM_SIZE_A) * $(DIM_SIZE_B) * $(NUM_FRAMES) )))
WINDOW_VSIZE_A = $(shell echo $$(( $(DIM_SIZE_A) * $(NUM_FRAMES) )) )
WINDOW_VSIZE_B = $(shell echo $$(( $(DIM_SIZE_B) * $(NUM_FRAMES) )) )

ifeq ($(T_DATA_A), int16)
	ifeq ($(T_DATA_B), int16)
		T_DATA_OUT := int16
	else ifeq ($(T_DATA_B), int32)
		T_DATA_OUT := int32
	else ifeq ($(T_DATA_B), cint16)
		T_DATA_OUT := cint16
	else ifeq ($(T_DATA_B), cint32)
		T_DATA_OUT := cint32
	endif
    
else ifeq ($(T_DATA_A), int32)
	ifeq ($(T_DATA_B), int16)
		T_DATA_OUT := int32
	else ifeq ($(T_DATA_B), int32)
		T_DATA_OUT := int32
	else ifeq ($(T_DATA_B), cint16)
		T_DATA_OUT := cint32
	else ifeq ($(T_DATA_B), cint32)
		T_DATA_OUT := cint32
	endif

else ifeq ($(T_DATA_A), cint16)
	ifeq ($(T_DATA_B), int16)
		T_DATA_OUT := cint16
	else ifeq ($(T_DATA_B), int32)
		T_DATA_OUT := cint32
	else ifeq ($(T_DATA_B), cint16)
		T_DATA_OUT := cint16
	else ifeq ($(T_DATA_B), cint32)
		T_DATA_OUT := cint32
	endif

else ifeq ($(T_DATA_A), cint32)
	T_DATA_OUT := cint32

else ifeq ($(T_DATA_A), float)
	ifeq ($(T_DATA_B), float)
		T_DATA_OUT := float
	else ifeq ($(T_DATA_B), cfloat)
		T_DATA_OUT := cfloat
	endif
else ifeq ($(T_DATA_A), cfloat)
	T_DATA_OUT := cfloat

endif

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) T_DATA_A $(T_DATA_A) T_DATA_B $(T_DATA_B) DIM_SIZE_A $(DIM_SIZE_A) DIM_SIZE_B $(DIM_SIZE_B) NUM_FRAMES $(NUM_FRAMES) SHIFT $(SHIFT) UUT_SSR $(UUT_SSR) API_IO $(API_IO) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE)

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_config:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

create_input:
	@echo starting create_input
	@echo LOC_INPUT_FILE_A $(LOC_INPUT_FILE_A)
	@echo LOC_INPUT_FILE_B $(LOC_INPUT_FILE_B)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(WINDOW_VSIZE_A) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(T_DATA_A) $(API_IO) 1 0 0 0 0 0 64 ;\
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(WINDOW_VSIZE_B) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(T_DATA_B) $(API_IO) 1 0 0 0 0 0 64 ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(WINDOW_VSIZE_A) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(T_DATA_A) $(API_IO) 1 0 0 0 0 0 64 ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(WINDOW_VSIZE_B) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(T_DATA_B) $(API_IO) 1 0 0 0 0 0 64 ;\
    perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl --file $(LOC_INPUT_FILE_A) --rows $(DIM_SIZE_A) --ssrSplit $(UUT_SSR) --split --type $(T_DATA_A) --NITER $(NITER) --plioWidth 64 ;\
	echo Input ready

sim_ref:
	make UUT_KERNEL=outer_tensor_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

get_diff:
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(UUT_SIM_FILE) --rows $(shell echo $$(( $(DIM_SIZE_A) * $(DIM_SIZE_B) ))) --cols 1 --ssrSplit $(UUT_SSR) --casc 1 --zip -t $(T_DATA_A) -findOutType $(T_DATA_B) -n $(NITER) --plioWidth 64 ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_A_0_0.txt ./data/uut_output_0.txt $(STATUS_FILE) $(DIM_OUT) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(DIM_OUT) 1 $(STATUS_FILE) ./aiesimulator_output "outer_tensor_main" $(NITER)
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
