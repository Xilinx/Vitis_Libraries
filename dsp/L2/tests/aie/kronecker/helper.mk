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

#################################################################################
# Makefile helper used for Kronecker matrix product, simulation and QoR harvest.
#################################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

HELPER:= $(HELPER_CUR_DIR)/.HELPER
SEED ?= 1
DIM_A_SIZE =  $(shell echo $$(( $(DIM_A_ROWS) * $(DIM_A_COLS) )))
DIM_B_SIZE =  $(shell echo $$(( $(DIM_B_ROWS) * $(DIM_B_COLS) )))
DIM_OUT_SIZE =  $(shell echo $$(( $(DIM_A_SIZE) * $(DIM_B_SIZE) )))
WINDOW_VSIZE_A 	= $(shell echo $$(( $(DIM_A_SIZE) * $(NUM_FRAMES))))
WINDOW_VSIZE_B 	= $(shell echo $$(( $(DIM_B_SIZE) * $(NUM_FRAMES))))
WINDOW_VSIZE_OUT =  $(shell echo $$(( $(DIM_OUT_SIZE) * $(NUM_FRAMES))))



NUM_PORTS := 1
CASC_LEN := 1


STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) T_DATA_A $(T_DATA_A) T_DATA_B $(T_DATA_B) DIM_A_ROWS $(DIM_A_ROWS) DIM_A_COLS $(DIM_A_COLS) DIM_B_ROWS $(DIM_B_ROWS) DIM_B_COLS $(DIM_B_COLS) NUM_FRAMES $(NUM_FRAMES) API_IO $(API_IO) SHIFT $(SHIFT) UUT_SSR $(UUT_SSR) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE)

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_input:
	@echo helper.mk stage:	create_input
	@echo NUM_PORTS $(NUM_PORTS) 
	@echo LOC_INPUT_FILE_A $(LOC_INPUT_FILE_A)
	@echo LOC_INPUT_FILE_B $(LOC_INPUT_FILE_B)

	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(WINDOW_VSIZE_A) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(T_DATA_A) $(API_IO) 1 0 0 0 0 0 64 ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(WINDOW_VSIZE_B) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(T_DATA_B) $(API_IO) 1 0 0 0 0 0 64 ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -file $(LOC_INPUT_FILE_A) --rows $(DIM_A_SIZE) --cols 1 --ssrSplit $(UUT_SSR) --casc $(CASC_LEN) --split -type $(T_DATA_A) --NITER $(NITER) --plioWidth 64 

	echo Input ready

sim_ref:
	@echo helper.mk stage:	sim_ref
	make UUT_KERNEL=kronecker_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF 
 
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
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f ./data/uut_output.txt --rows $(DIM_OUT_SIZE) --cols 1 --ssrSplit $(UUT_SSR) --casc $(CASC_LEN) --zip -type $(T_DATA_A) -findOutType $(T_DATA_B) -n $(NITER) --plioWidth 64;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:

	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_inputA_0_0.txt ./data/uut_output_0.txt $(STATUS_FILE) $(WINDOW_VSIZE_OUT) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE_OUT) 1 $(STATUS_FILE) ./aiesimulator_output "kronecker_main" $(NITER)
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts


create_config:
	echo $(STATUS_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
