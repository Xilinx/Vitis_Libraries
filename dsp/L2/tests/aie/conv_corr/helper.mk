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
# Makefile helper used for conv_corr compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
HELPER:= $(HELPER_CUR_DIR)/.helper
SEED_DATA_G ?= 0
SEED_DATA_F ?= 0
USE_RTP_VECTOR_LENGTHS ?= 0
ceil = $(shell echo $$(((($1 + $2 - 1)/ $2) * $2)))
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_F $(DATA_F) DATA_G $(DATA_G) DATA_OUT $(DATA_OUT) FUNCT_TYPE $(FUNCT_TYPE) COMPUTE_MODE $(COMPUTE_MODE) F_LEN $(F_LEN) G_LEN $(G_LEN) SHIFT $(SHIFT) API_IO $(API_IO) RND $(RND) SAT $(SAT) NUM_FRAMES $(NUM_FRAMES) CASC_LEN $(CASC_LEN) PHASES $(PHASES) USE_RTP_VECTOR_LENGTHS $(USE_RTP_VECTOR_LENGTHS)

ifeq ($(DATA_F), int8)
	ifeq ($(DATA_G), int8)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 32
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 32
		endif
	endif
endif

ifeq ($(DATA_F), int16)
	ifeq ($(DATA_G), int8)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), int16)
	ifeq ($(DATA_G), int16)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), int32)
	ifeq ($(DATA_G), int16)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), float)
	ifeq ($(DATA_G), float)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), float)
	ifeq ($(DATA_G), cfloat)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), cint16)
	ifeq ($(DATA_G), int16)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), cint16)
	ifeq ($(DATA_G), int32)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), cint16)
	ifeq ($(DATA_G), cint16)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif


ifeq ($(DATA_F), cint32)
	ifeq ($(DATA_G), int16)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), cint32)
	ifeq ($(DATA_G), cint16)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), cfloat)
	ifeq ($(DATA_G), float)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), cfloat)
	ifeq ($(DATA_G), cfloat)
	    ifeq ($(AIE_VARIANT), 1)
		    NUM_LANES := 8
		else ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif

ifeq ($(DATA_F), bfloat16)
	ifeq ($(DATA_G), bfloat16)
	    ifeq ($(AIE_VARIANT), 2)
		    NUM_LANES := 16
		else ifeq ($(AIE_VARIANT), 22)
		    NUM_LANES := 16
		endif
	endif
endif


ifeq ($(COMPUTE_MODE), 0)
    OUT_LEN := $(shell echo $$((( ( $(F_LEN) + $(G_LEN)) - 1))))
else ifeq ($(COMPUTE_MODE),1)
    OUT_LEN := $(F_LEN)
else ifeq ($(COMPUTE_MODE),2)
    OUT_LEN := $(shell echo $$((( ( $(F_LEN) - $(G_LEN)) + 1))))
else
    OUT_LEN := $(shell echo $$((( ( $(F_LEN) + $(G_LEN)) - 1))))
endif

CEIL_RES = $(call ceil, $(OUT_LEN), $(NUM_LANES))
ifeq ($(API_IO),1)
    OUT_DATA_LEN := $(F_LEN)
else
    OUT_DATA_LEN := $(shell echo $$((  $(CEIL_RES)*$(NUM_FRAMES))))
endif

DIFF_TOLERANCE = 0.0025
ifeq ($(DATA_F), float)
CC_TOLERANCE = 0.0025
else ifeq ($(DATA_F), cfloat)
CC_TOLERANCE = 0.0025
else
CC_TOLERANCE = 0
endif

REF_F_LEN         = $(F_LEN)
REF_G_LEN         = $(G_LEN)
NITER_UUT         = $(NITER)
NITER_REF         = $(NITER_UUT)
ifeq ($(API_IO), 1)
  NITER_REF      := 1
  REF_F_LEN      := $(shell echo ${F_LEN}*${NITER_UUT} | bc )
endif


$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_config:
	echo creating configuration;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

create_input:
	@echo starting generation of input
	@if [ $(API_IO) == 1 ]; then \
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/gen_input.tcl $(LOC_INPUT_FILE_F) $(F_LEN) $(NITER_UUT) $(SEED_DATA_F) $(STIM_TYPE_F) 0 0 $(DATA_F) $(API_IO) 1 0 0 $(DATA_G) 0 0 ; \
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/gen_input_f32_bf16.tcl $(LOC_INPUT_FILE_G) $(G_LEN) $(NITER_UUT) $(SEED_DATA_G) $(STIM_TYPE_G) 0 0 $(DATA_G) $(API_IO) 1 0 0 $(DATA_G) 0 0 $(NUM_FRAMES) 1; \
    else \
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/gen_padded_input.tcl $(LOC_INPUT_FILE_F) $(F_LEN) $(NITER_UUT) $(SEED_DATA_F) $(STIM_TYPE_F) 0 0 $(DATA_F) $(API_IO) 1 0 0 $(DATA_G) 0 0 $(COMPUTE_MODE) $(G_LEN) $(AIE_VARIANT) $(NUM_FRAMES) ; \
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/gen_input_f32_bf16.tcl $(LOC_INPUT_FILE_G) $(G_LEN) $(NITER_UUT) $(SEED_DATA_G) $(STIM_TYPE_G) 0 0 $(DATA_G) $(API_IO) 1 0 0 $(DATA_G) 0 0 $(NUM_FRAMES) 0 ; \
	fi; \
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/split_interleave.tcl $(LOC_INPUT_FILE_F) $(DATA_F) $(PHASES) ; \
    echo Input ready

sim_ref:
	@echo starting sim_ref;\
	UUT_KERNEL=conv_corr_ref UUT_SIM_FILE=./data/ref_output.txt make run TARGET=x86sim TAG=REF

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
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_OUT) --ssr $(PHASES) --zip --dual 0 -k 0 -w ${OUT_DATA_LEN} ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_OUT) --ssr 1 --zip --dual 0 -k 0 -w ${OUT_DATA_LEN} ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/conv_corr/align_stream_output.tcl $(REF_SIM_FILE) $(UUT_SIM_FILE) $(PHASES) $(CASC_LEN) $(REF_F_LEN) $(G_LEN) 1 $(API_IO) $(DATA_G);\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl $(UUT_SIM_FILE) $(REF_SIM_FILE) ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) PERCENT

get_latency:
	@sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT);\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_inData_F_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(F_LEN) $(NITER_UUT)

get_stats:
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(F_LEN) 1 $(STATUS_FILE) ./aiesimulator_output "conv_corrMain" $(NITER_UUT)

get_status:
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

harvest_mem:
	@$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts

cleanup:
	make cleanall