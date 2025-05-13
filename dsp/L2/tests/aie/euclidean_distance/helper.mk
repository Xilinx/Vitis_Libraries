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
# Makefile helper used for euclidean_distance compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
HELPER:= $(HELPER_CUR_DIR)/.helper
SEED_DATA ?= 1


ceil = $(shell echo $$(((($1 + $2 - 1)/ $2) * $2)))

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA $(DATA) DATA_OUT $(DATA_OUT) LEN $(LEN)  DIM $(DIM) API_IO $(API_IO) RND $(RND) SAT $(SAT) IS_OUTPUT_SQUARED $(IS_OUTPUT_SQUARED)

DIFF_TOLERANCE = 0.0025
ifeq ($(DATA), float)
CC_TOLERANCE = 0.0025
else ifeq ($(DATA), cfloat)
CC_TOLERANCE = 0.0025
endif

FIXED_DIM = 4
IS_ZERO_PADDING_REQUIRED = 0
ifneq ($(DIM) , $(FIXED_DIM))
  IS_ZERO_PADDING_REQUIRED = 1
endif

NITER_UUT         = $(NITER)
NITER_REF         = $(NITER_UUT)

REQUIRED_LEN_P    = $(shell echo $$((  $(LEN)*$(DIM))))
REQUIRED_LEN_Q    = $(shell echo $$((  $(LEN)*$(DIM))))
REARRANGED_LEN_P    = $(shell echo $$((  $(LEN)*$(FIXED_DIM))))
REARRANGED_LEN_Q    = $(shell echo $$((  $(LEN)*$(FIXED_DIM))))

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_config:
	echo creating configuration;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

validate_config:
	echo validating configuration;\
	vitis --classic -exec ipmetadata_config_checker $(HELPER_ROOT_DIR)/L2/meta/euclidean_distance.json ./config.json -newflow

create_input:
	@echo starting generation of input
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/euclidean_distance/gen_input_ED.tcl $(LOC_INPUT_FILE_P) $(REQUIRED_LEN_P) $(NITER_UUT) $(SEED_DATA) $(STIM_TYPE) 0 0 $(DATA) $(API_IO) 1 0 0 $(DATA) 0 0 $(NUM_FRAMES) 1;\
    tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/euclidean_distance/gen_input_ED.tcl $(LOC_INPUT_FILE_Q) $(REQUIRED_LEN_Q) $(NITER_UUT) $(SEED_DATA) $(STIM_TYPE) 0 0 $(DATA) $(API_IO) 1 0 0 $(DATA) 0 0 $(NUM_FRAMES) 1;\
	
	@if [ $(IS_ZERO_PADDING_REQUIRED) == 1 ]; then \
	    echo Input rearrange starts;\
	    tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/euclidean_distance/rearrangeInData.tcl $(LOC_INPUT_FILE_P) $(DATA) $(DIM) $(FIXED_DIM) ; \
	    tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/euclidean_distance/rearrangeInData.tcl $(LOC_INPUT_FILE_Q) $(DATA) $(DIM) $(FIXED_DIM) ; \
	fi;\
    echo Input ready

sim_ref:
	@echo starting sim_ref;\
	UUT_KERNEL=euclidean_distance_ref UUT_SIM_FILE=./data/ref_output.txt make run TARGET=x86sim TAG=REF

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

	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl $(UUT_SIM_FILE) $(REF_SIM_FILE) ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) PERCENT

get_latency:
	@sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT);\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_inData_P.txt ./data/uut_output.txt $(STATUS_FILE) $(LEN) $(NITER_UUT)

get_stats:
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(LEN) 1 $(STATUS_FILE) ./aiesimulator_output "euclideanDistMain" $(NITER_UUT)

get_status:
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

harvest_mem:
	@$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

cleanup:
	make cleanall