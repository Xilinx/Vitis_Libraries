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
SEED_DATA_G ?= 1
SEED_DATA_F ?= 1

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_F $(DATA_F) DATA_G $(DATA_G) DATA_OUT $(DATA_OUT) FUNCT_TYPE $(FUNCT_TYPE) COMPUTE_MODE $(COMPUTE_MODE) F_LEN $(F_LEN) G_LEN $(G_LEN) SHIFT $(SHIFT) API_PORT $(API_PORT) RND $(RND) SAT $(SAT) 
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

$(HELPER):
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_padded_input.tcl $(LOC_INPUT_FILE_F) $(F_LEN) $(NITER) $(SEED_DATA_F) $(STIM_TYPE_F) 0 0 $(DATA_F) $(API_PORT) 1 0 0 $(DATA_G) 0 0 $(COMPUTE_MODE) $(F_LEN) $(G_LEN) $(AIE_VARIANT) ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_G) $(G_LEN) $(NITER) $(SEED_DATA_G) $(STIM_TYPE_G) 0 0 $(DATA_G) $(API_PORT) 1 0 0 0 0 0 ;\
	UUT_KERNEL=conv_corr_ref UUT_SIM_FILE=./data/ref_output.txt make run TARGET=x86sim TAG=REF ;\
	make cleanall

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_F.txt ./data/uut_output.txt $(STATUS_FILE) $(F_LEN) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(F_LEN) 1 $(STATUS_FILE) ./aiesimulator_output "conv_corrMain" $(NITER) 

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

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
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl $(UUT_SIM_FILE) $(REF_SIM_FILE) ./logs/diff.txt

create_config:
	echo $(STATUS_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts