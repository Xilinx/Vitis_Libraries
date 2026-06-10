#
# Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
# Makefile helper for substitution compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
COMMON_SCRIPTS_DIR = $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

OUTPUT_VSIZE := $(shell echo $$(( $(DIM_SIZE) * $(NUM_FRAMES) )) )
NUM_MATRICES = $(shell echo $$(( $(NITER) * $(NUM_FRAMES) )) )
SUB_MATRIX_DIM = $(shell echo $$(( $(DIM_SIZE) / $(GRID_DIM) )) )

LOC_INPUT_FILE_L ?= ./data/in_L.txt
LOC_INPUT_FILE_y ?= ./data/in_y.txt
LOC_REF_FILE_x ?= ./data/ref_x.txt

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DIM_SIZE $(DIM_SIZE) SUBST_TYPE $(SUBST_TYPE) L_LEADING $(L_LEADING) GRID_DIM $(GRID_DIM) NUM_FRAMES $(NUM_FRAMES) DIAG_INV $(DIAG_INV)
DATA_SEED ?= 0
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
HELPER:= $(HELPER_CUR_DIR)/.HELPER

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3

$(HELPER): sim_ref prep_x86_out
	make cleanall

create_config:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

create_input:
	rm -f $(LOC_INPUT_FILE_L) $(LOC_INPUT_FILE_y) $(LOC_REF_FILE_x)
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/substitution/gen_input_substitution.py --data_type $(DATA_TYPE) --dim_size $(DIM_SIZE) --subst_type $(SUBST_TYPE) --l_leading $(L_LEADING) --grid_dim 1 --seed $(DATA_SEED) --loc_in_L $(LOC_INPUT_FILE_L) --loc_in_y $(LOC_INPUT_FILE_y) --loc_ref_x $(LOC_REF_FILE_x) --niter $(NUM_MATRICES)
	perl $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(LOC_INPUT_FILE_L) --rows $(DIM_SIZE) --cols $(DIM_SIZE) --ssrSplit $(GRID_DIM) --casc $(GRID_DIM) --split -t $(DATA_TYPE) --NITER $(NUM_MATRICES) --colMajor 1 --plioWidth 64;\
	perl $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(LOC_INPUT_FILE_Y) --rows $(DIM_SIZE) --cols 1 --ssrSplit $(GRID_DIM) --casc 1 --split -t $(DATA_TYPE) --NITER $(NUM_MATRICES) --colMajor 1 --plioWidth 64;\

sim_ref:
	echo "Starting sim_ref";\
	make UUT_KERNEL=substitution_ref run TARGET=x86sim TAG=REF ;\
	echo "Ending sim_ref"

prep_x86_out:
	rm -f ./data/uut_output.txt
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/simple_interlace.py $(SUB_MATRIX_DIM) $(DATA_TYPE) $(GRID_DIM) $(NUM_MATRICES) ./data/uut_output ./data/uut_output.txt

prep_aie_out:
	rm -f ./data/uut_output.txt
	@aie_raw_files=`find $(HELPER_CUR_DIR)/aiesimulator_output/data -name "uut_output*" -type f | sort -V`;\
	echo "AIE raw files= " $$aie_raw_files;\
	for f in $$aie_raw_files; do \
		grep -ve '[XT]' $$f > $(HELPER_CUR_DIR)/data/`basename $$f`;\
	done
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/simple_interlace.py $(SUB_MATRIX_DIM) $(DATA_TYPE) $(GRID_DIM) $(NUM_MATRICES) ./data/uut_output ./data/uut_output.txt

diff:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) 

get_status:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) 

get_latency:
	sh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)

	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py -t_in_file T_in_L_0_0.txt -t_out_file uut_output_0_0.txt -status_file_dir $(STATUS_FILE) -num_of_samples $(OUTPUT_VSIZE) -niter $(NITER) -use_outputs_if_no_inputs False -casc_len 1 -aiesim_out_dir ./aiesimulator_output -ip substitution

cleanall:
	rm -rf aiesimulator_output x86simulator_output Work data .Xil vitis_analyzer*
