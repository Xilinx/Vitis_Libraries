#
# Copyright 2021 Xilinx, Inc.
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


GOLDEN:= $(GOLDEN_CUR_DIR)/.golden

$(GOLDEN):
	tclsh $(GOLDEN_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE_A) $(P_INPUT_WINDOW_VSIZE_A) $(NITER_UUT) 1 $(STIM_TYPE_A)
	tclsh $(GOLDEN_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE_B) $(P_INPUT_WINDOW_VSIZE_B) $(NITER_UUT) 2 $(STIM_TYPE_B)
	tclsh $(GOLDEN_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(P_INPUT_WINDOW_VSIZE_A) $(NITER_UUT) 1 $(STIM_TYPE_A)
	tclsh $(GOLDEN_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(P_INPUT_WINDOW_VSIZE_B) $(NITER_UUT) 2 $(STIM_TYPE_B)
	TARGET=x86sim UUT_KERNEL=matrix_mult_ref UUT_SIM_FILE=./data/ref_output.txt make x86sim TARGET=x86sim TAG=REF
	make cleanall
	touch $(GOLDEN)
