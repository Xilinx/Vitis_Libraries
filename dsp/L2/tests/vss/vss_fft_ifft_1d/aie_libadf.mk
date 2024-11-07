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
AIE_CXXFLAGS += --target=$(AIETARGET) --platform=$(XPLATFORM)  
AIE_CXXFLAGS += $(AIE_CXXFLAGS_INC)
AIE_CONTAINER = libadf.a
AIE_WORK_DIR = $(CUR_DIR)/Work
AIE_PKG_DIR = $(AIE_WORK_DIR)

ifneq (,$(filter aiesim x86sim , $(TARGET)))
ifneq (,$(filter REF, $(TAG)))
$(AIE_CONTAINER): pre_build test.cpp uut_config.h $(XFLIB_DIR)/L1/src/aie/fft_window.cpp $(XFLIB_DIR)/L1/src/aie/fft_ifft_dit_1ch.cpp 
	$(ECHO) "Compiling: REF libadf.a"
	mkdir -p $(dir $@)
	$(VPP) -c --mode aie $(AIE_CXXFLAGS) -o $@ $(filter %.s %.c %.cc %.cpp %cxx, $^)
else
$(AIE_CONTAINER): pre_build
	make -f  $(XFLIB_DIR)/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d.mk libadf AIETARGET=$(TARGET) HELPER_ROOT_DIR=$(XFLIB_DIR) HELPER_CUR_DIR=$(CUR_DIR)
endif
endif