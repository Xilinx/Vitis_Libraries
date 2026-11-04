#
# Copyright (C) 2026, Advanced Micro Devices, Inc.
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
# Makefile helper used for ls_cholesky_solver compilation, simulation and
# QoR harvest.
###############################################################################

HELPER_CUR_DIR  ?= .
HELPER_ROOT_DIR ?= ./../../../../
COMMON_SCRIPTS_DIR = $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DIM_SIZE $(DIM_SIZE) \
            NUM_FRAMES $(NUM_FRAMES) GRID_DIM $(GRID_DIM) CASC_LEN $(CASC_LEN)
EXTRA_PARAM_MAP = NITER $(NITER)

NUM_MATRICES    = $(shell echo $$(( $(NITER) * $(NUM_FRAMES) )) )
SUB_MATRIX_DIM  = $(shell echo $$(( $(DIM_SIZE) / $(GRID_DIM) )) )
SUB_MATRIX_SIZE = $(shell echo $$(( $(SUB_MATRIX_DIM) * $(SUB_MATRIX_DIM) )) )
INPUT_WINDOW_VSIZE = $(shell echo $$(( $(SUB_MATRIX_SIZE) * $(NUM_FRAMES) )) )
NUM_PORTS = $(shell echo $$(( ($(GRID_DIM) * ($(GRID_DIM) + 1)) / 2 )) )
NUM_OF_SAMPLES = $(shell echo $$(( $(NUM_PORTS) * $(INPUT_WINDOW_VSIZE) )) )
NUM_OUTPUT_PORTS= $(GRID_DIM)

LOC_INPUT_FILE_A   ?= ./data/in_A.txt
LOC_INPUT_FILE_Y   ?= ./data/in_y.txt
LOC_REF_FILE_X     ?= ./data/ref_x.txt
LOC_FULL_A         ?= ./data/in_A_full.txt
LOC_FULL_Y         ?= ./data/in_y_full.txt
DATA_SEED          ?= 1

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
STATUS_JSON = ./logs/status_$(UUT_KERNEL)_$(PARAMS).json

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3

HELPER := $(HELPER_CUR_DIR)/.HELPER

# Default target: prepare the validation output (Python numpy golden reference),
# compile and run the reference model under x86sim, collect the reference output,
# then clean intermediate artefacts so the UUT build starts from a clean state.
$(HELPER): prep_validation_out sim_ref prep_ref_out
	make cleanall

create_config:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl \
	    ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)

create_input:
	rm -f $(LOC_INPUT_FILE_A) $(LOC_INPUT_FILE_Y) $(LOC_REF_FILE_X) \
	      $(LOC_FULL_A) $(LOC_FULL_Y)
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/ls_cholesky_solver/gen_input_ls_cholesky_solver.py \
	    --data_type  $(DATA_TYPE) \
	    --dim_size   $(DIM_SIZE) \
	    --grid_dim   $(GRID_DIM) \
	    --seed       $(DATA_SEED) \
	    --loc_in_A   $(LOC_INPUT_FILE_A) \
	    --loc_in_y   $(LOC_INPUT_FILE_Y) \
	    --loc_ref_x  $(LOC_REF_FILE_X) \
	    --loc_full_A $(LOC_FULL_A) \
	    --loc_full_y $(LOC_FULL_Y) \
	    --niter      $(NUM_MATRICES)

# Stitch the per-port numpy golden-reference files produced by create_input into
# a single validation_output.txt.  This file is used as a mathematical sanity
# check independently of the AIE reference model simulation.
# Each ref_x_<gx>_0.txt holds the numpy solution slice for GRID_DIM column gx.
prep_validation_out:
	mkdir -p ./data
	rm -f ./data/validation_output.txt
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/simple_interlace.py \
	    $(SUB_MATRIX_DIM) $(DATA_TYPE) $(NUM_OUTPUT_PORTS) $(NUM_MATRICES) \
	    $(LOC_REF_FILE_X:.txt=) ./data/validation_output.txt

# Compile and run the ls_cholesky_solver reference model under x86sim.
# TAG=REF selects REF_PREPROC_ARGS in the Makefile, which sets:
#   UUT_GRAPH=ls_cholesky_solver_ref_graph, USING_UUT=0,
#   INPUT_FILE_A=LOC_FULL_A (full non-tiled matrix), INPUT_FILE_Y=LOC_FULL_Y,
#   OUTPUT_FILE=REF_SIM_FILE (./data/ref_output.txt).
# With USING_UUT=0, test.hpp activates the single-port reference graph path.
sim_ref:
	echo "Starting sim_ref";\
	make UUT_KERNEL=ls_cholesky_solver_ref run TARGET=x86sim TAG=REF ;\
	echo "Ending sim_ref"

# Collect the single-file reference output from x86sim (the ref graph writes the
# full solution vector to OUTPUT_FILE with no per-port suffix) and strip the
# simulator timestamp/marker lines to produce the clean ref_output.txt used by
# diff.tcl.
prep_ref_out:
	mkdir -p ./data
	rm -f ./data/ref_output.txt
	grep -ve '[XT]' \
	    $(HELPER_CUR_DIR)/x86simulator_output/data/ref_output.txt \
	    > $(HELPER_CUR_DIR)/data/ref_output.txt

prep_x86_out:
	rm -f ./data/uut_output.txt
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
	    grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n \
	        > $(HELPER_CUR_DIR)/data/$$n;\
	done
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/simple_interlace.py \
	    $(SUB_MATRIX_DIM) $(DATA_TYPE) $(NUM_OUTPUT_PORTS) $(NUM_MATRICES) \
	    ./data/uut_output ./data/uut_output.txt

prep_aie_out:
	rm -f ./data/uut_output.txt
	@aie_raw_files=`find $(HELPER_CUR_DIR)/aiesimulator_output/data \
	    -name "uut_output*" -type f | sort -V`;\
	echo "AIE raw files= " $$aie_raw_files;\
	for f in $$aie_raw_files; do \
	    grep -ve '[XT]' $$f \
	        > $(HELPER_CUR_DIR)/data/`basename $$f`;\
	done
	$(VITIS_PYTHON3) $(COMMON_SCRIPTS_DIR)/simple_interlace.py \
	    $(SUB_MATRIX_DIM) $(DATA_TYPE) $(NUM_OUTPUT_PORTS) $(NUM_MATRICES) \
	    ./data/uut_output ./data/uut_output.txt

diff:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl \
	    ./data/uut_output.txt ./data/ref_output.txt \
	    ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py functional_check $(STATUS_JSON) ./logs/diff.txt
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_ls_cholesky_solver.py \
	    --A          $(LOC_FULL_A) \
	    --x          ./data/uut_output.txt \
	    --y          $(LOC_FULL_Y) \
	    --dim_size   $(DIM_SIZE) \
	    --data_type  $(DATA_TYPE) \
	    --num_frames $(NUM_FRAMES) \
	    --niter      $(NITER) \
	    --model_ut   "UUT"

get_status:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl \
	    $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) $(EXTRA_PARAM_MAP)
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py init $(STATUS_JSON) UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) $(EXTRA_PARAM_MAP)

get_latency:
	sh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh \
	    $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py \
	    -t_in_file  T_in_A_0_0.txt \
	    -t_out_file uut_output_0_0.txt \
	    -status_file_dir $(STATUS_FILE) \
	    -num_of_samples $(NUM_OF_SAMPLES) \
	    -niter $(NITER) \
	    -use_outputs_if_no_inputs False \
	    -casc_len $(CASC_LEN) \
	    -aiesim_out_dir ./aiesimulator_output \
	    -ip cholesky
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py qor $(STATUS_JSON) Work $(NITER) $(NUM_FRAMES) T_in_ uut_output_ 64 $(DATA_TYPE) --verbose

get_memory:
	$(VITIS_PYTHON3) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/status_json/update_status.py harvest $(STATUS_JSON) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts Work

cleanall:
	rm -rf aiesimulator_output x86simulator_output Work data .Xil vitis_analyzer*
