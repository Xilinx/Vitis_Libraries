#
# Copyright 2019 Xilinx, Inc.
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


# This is a TOP-LEVEL makefile that can be used to build all of the
# examples

# unused but required for makefile compilance checking
SRCS=
OBJ_FILES=
EXTRA_OBJS=
EXE_NAME=
EXE_EXT=
EXE_FILE=
HOST_ARGS=
RUN_ENV=


# search one level down for directories containing a makfile
SUBDIRS = $(dir $(shell find ./*/ -name "Makefile"))

$(info SUBDIRS are $(SUBDIRS))

.PHONY: all clean cleanall

all: $(SUBDIRS)
clean: $(SUBDIRS)
cleanall: $(SUBDIRS)
run:
	@echo "please run each example within its own directory"

$(SUBDIRS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
