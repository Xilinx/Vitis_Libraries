#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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
# Makefile helper used for Cholesky compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
COMMON_SCRIPTS_DIR = $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DIM_SIZE $(DIM_SIZE) NUM_FRAMES $(NUM_FRAMES) GRID_DIM $(GRID_DIM) CASC_LEN $(CASC_LEN) DIAG_INV $(DIAG_INV)
EXTRA_PARAM_MAP = NITER $(NITER)
NUM_MATRICES = $(shell echo $$(( $(NITER) * $(NUM_FRAMES) )) )
SUB_MATRIX_DIM = $(shell echo $$(( $(DIM_SIZE) / $(GRID_DIM) )) )
SUB_MATRIX_SIZE = $(shell echo $$(( $(SUB_MATRIX_DIM) * $(SUB_MATRIX_DIM) )) )
INPUT_WINDOW_VSIZE = $(shell echo $$(( $(SUB_MATRIX_SIZE) * $(NUM_FRAMES) )) )
NUM_PORTS = $(shell echo $$(( ($(GRID_DIM) * ($(GRID_DIM) + 1)) / 2 )) )
NUM_OF_SAMPLES = $(shell echo $$(( $(NUM_PORTS) * $(INPUT_WINDOW_VSIZE) )) )
LAST_PORT = $(shell echo $$(( $(GRID_DIM) - 1 )))_$(shell echo $$(( $(GRID_DIM) - 1 )))
DATA_SEED ?= 0
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
SPLIT_ZIP_FILE ?=

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3

sim_ref:
	make run TARGET=x86sim TAG=REF ;\
	make cleanall

gen_input:
	tclsh $(COMMON_SCRIPTS_DIR)/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/cholesky/gen_input_cholesky.py $(INPUT_FILE) $(DATA_TYPE) $(DIM_SIZE) $(NUM_MATRICES) $(DATA_SEED);\
	perl $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_card_cut.pl -f $(INPUT_FILE) --rows $(DIM_SIZE) --cols $(DIM_SIZE) --ssrSplit $(GRID_DIM) --casc $(GRID_DIM) --split -t $(DATA_TYPE) --NITER $(NUM_MATRICES) --colMajor 1 --plioWidth 64;\

prep_x86_out:
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/stitch_inputs.py $(SUB_MATRIX_DIM) $(SUB_MATRIX_DIM) $(GRID_DIM) $(GRID_DIM) $(NUM_MATRICES) $(DATA_TYPE) --COL_MAJOR 1 --HANDLE_MISSING 1;\
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/postprocessing.py $(HELPER_CUR_DIR)/data/uut_output.txt $(HELPER_CUR_DIR)/data/uut_output.txt $(DATA_TYPE)

prep_aie_out:
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/stitch_inputs.py $(SUB_MATRIX_DIM) $(SUB_MATRIX_DIM) $(GRID_DIM) $(GRID_DIM) $(NUM_MATRICES) $(DATA_TYPE) --COL_MAJOR 1 --HANDLE_MISSING 1;\
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/postprocessing.py $(HELPER_CUR_DIR)/data/uut_output.txt $(HELPER_CUR_DIR)/data/uut_output.txt $(DATA_TYPE)

diff:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(DIFF_TOLERANCE) abs
	if [ "$(DIAG_INV)" = "0" ]; then \
		$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_cholesky.py --A ./data/input.txt --L ./data/ref_output.txt --dim_size $(DIM_SIZE) --data_type $(DATA_TYPE) --num_frames $(NUM_FRAMES) --niter $(NITER) --model_ut "Ref"; \
	fi

get_status:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) $(EXTRA_PARAM_MAP)

get_latency:
	sh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)

	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py -t_in_file T_input_$(LAST_PORT).txt -t_out_file uut_output_$(LAST_PORT).txt -status_file_dir $(STATUS_FILE) -num_of_samples $(NUM_OF_SAMPLES) -niter $(NITER) -use_outputs_if_no_inputs False -casc_len $(CASC_LEN) -aiesim_out_dir ./aiesimulator_output -ip cholesky
