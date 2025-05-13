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
# Makefile helper used for bitonic_sort compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
SEED ?= 0

WINDOW_VSIZE = $$(( $(DIM_SIZE) * $(NUM_FRAMES) ))

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DIM_SIZE $(DIM_SIZE) NUM_FRAMES $(NUM_FRAMES) ASCENDING $(ASCENDING) CASC_LEN $(CASC_LEN) UUT_SSR $(UUT_SSR)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_input:
	@echo starting create_input
	@echo LOC_INPUT_FILE $(LOC_INPUT_FILE)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) 0 0 $(DATA_TYPE) 0 1 0 0 0 0 0 64;\
    perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl --file $(LOC_INPUT_FILE) --rows $(DIM_SIZE) --ssrSplit $(UUT_SSR) --split --type $(DATA_TYPE) --NITER $(NITER) --plioWidth 64;\
	echo Input ready

sim_ref:
	make UUT_KERNEL=bitonic_sort_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF


get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

create_config:
	echo $(STATUS_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)

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
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(UUT_SIM_FILE) --rows  $(DIM_SIZE) --cols 1 --ssrSplit $(UUT_SSR) --casc 1 --zip -t $(DATA_TYPE) -findOutType $(DATA_TYPE) -n $(NITER) --plioWidth 64;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)


get_latency:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(VCD_FILE_DIR) $(STATUS_FILE) $(AIE_PART);\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output.txt $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(DIM_SIZE) 1 $(STATUS_FILE) ./aiesimulator_output "bitonic" $(NITER)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts