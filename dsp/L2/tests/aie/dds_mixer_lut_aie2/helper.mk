#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################
# Values for diff_tolerance are obtained experimentally. They are non-zero for streaming cases because
# number of lanes differs between reference model and uut. They are non-zero for ssr cases since uut and reference
# model split the lookup angle into differet sub-angles. Sincos lookup values of these sub-angles suffer from information
# loss due to rounding. Further, there is a rounding error from their multiplication.
HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
DIFF_TOLERANCE ?= 4
CC_TOLERANCE ?= 0

ifneq ($(UUT_SSR), 1)
	ifeq ($((expr SFDR \> 60)), 1)
		ifeq ($(DATA_TYPE), cint32)
			DIFF_TOLERANCE := 25750
		endif
		ifeq ($(DATA_TYPE), cint16)
			DIFF_TOLERANCE := 4
		endif
		ifeq ($(DATA_TYPE), cfloat)
			DIFF_TOLERANCE := 0.4
		endif	
	else
		ifeq ($(DATA_TYPE), cfloat)
			DIFF_TOLERANCE := 450
		endif
		ifeq ($(DATA_TYPE), cint32)
			DIFF_TOLERANCE := 26351012
		endif
		ifeq ($(DATA_TYPE), cint16)
			DIFF_TOLERANCE := 520
		endif	
	endif
endif

diff:
	@echo executing diff.tcl with new diff_tolerances
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./logs/uut_output.txt ./logs/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) abs
	@echo diff script done
