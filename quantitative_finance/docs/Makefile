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

ifeq (, $(shell which doxygen 2>/dev/null))
$(error "No doxygen in PATH.")
endif
ifeq (, $(shell which doxyrest 2>/dev/null))
$(error "No doxyrest in PATH.")
endif
ifeq (, $(shell which sphinx-build 2>/dev/null))
$(error "No sphinx-build in PATH.")
endif
ifeq (, $(HTML_DEST_DIR))
$(error "HTML_DEST_DIR is not set.")
endif

.PHONY: all install xml rst html clean cleanhtml cleanall

all: .stamp/sphinx

install: .stamp/sphinx
	rm -rf $(HTML_DEST_DIR)/*
#	make cleanall
	rsync -av --delete --exclude .git --exclude .nojekyll --exclude .buildinfo \
		 build/html/ $(HTML_DEST_DIR) 1>/dev/null

.stamp/doxygen:
	rm -rf xml_L1 xml_L2
	doxygen Doxyfile_L1
	doxygen Doxyfile_L2
	mkdir -p .stamp
	touch .stamp/doxygen


ifneq (,$(wildcard $(dir $(shell which doxyrest))/../doxyrest_frame/.))
# doxyrest built from sources
DOXYREST_FRAME = $(dir $(shell which doxyrest))/../doxyrest_frame
else ifneq (,$(wildcard $(dir $(shell which doxyrest))/../share/doxyrest/frame/.))
# doxyrest installed from binary distribution
DOXYREST_FRAME = $(dir $(shell which doxyrest))/../share/doxyrest/frame
endif

.stamp/doxyrest: .stamp/doxygen
	rm -rf src/rst_L1
	mkdir src/rst_L1
	doxyrest xml_L1/index.xml -o src/rst_L1/index.rst \
		-F $(DOXYREST_FRAME) -f c_index.rst.in -Dg_verbatimToCodeBlock
	rm -f src/rst_L1/index.rst src/rst_L1/global.rst
	rm -rf src/rst_L2
	mkdir src/rst_L2
	doxyrest xml_L2/index.xml -o src/rst_L2/index.rst \
		-F $(DOXYREST_FRAME) -f c_index.rst.in -Dg_verbatimToCodeBlock
	rm -f src/rst_L2/index.rst src/rst_L2/global.rst
	touch .stamp/doxyrest

.stamp/sphinx: .stamp/doxyrest Makefile.sphinx
	make -f Makefile.sphinx html
	touch .stamp/sphinx

xml:
	rm -f .stamp/doxygen
	make .stamp/doxygen

rst:
	rm -f .stamp/doxyrest
	make .stamp/doxyrest

html:
	rm -rf .stamp/sphinx build/doctrees
	make .stamp/sphinx

clean:
	rm -rf .stamp

cleanhtml:
	rm -rf .stamp/sphinx build/html build/doctrees

cleanall:
	rm -rf .stamp xml_L1 xml_L2 src/rst_L1 src/rst_L2 build/html build/doctrees
