#
# Copyright 2022 Xilinx, Inc.
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
ifneq ($(TARGET),$(filter $(TARGET), hw_emu))
#ifneq ($(findstring 2018, $(DEVICE)), 2018)
#	CXXFLAGS += -DFREE_RUNNING_KERNEL
#	VPP_FLAGS += -DFREE_RUNNING_KERNEL
#endif

ifeq ($(TARGET),$(filter $(TARGET), hw_emu))
	CXXFLAGS += -DFREE_RUNNING_KERNEL
	VPP_FLAGS += -DFREE_RUNNING_KERNEL
endif
endif
