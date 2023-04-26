#
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
# SPDX-License-Identifier: X11
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
# 
# Except as contained in this notice, the name of Advanced Micro Devices 
# shall not be used in advertising or otherwise to promote the sale,
# use or other dealings in this Software without prior written authorization 
# from Advanced Micro Devices, Inc.
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

all: install

install: .stamp/sphinx
	rm -rf $(HTML_DEST_DIR)/*
	rsync -av build/html/ $(HTML_DEST_DIR)

.stamp/doxygen:
	rm -rf xml_L1 xml_L2 xml_L3
	doxygen Doxyfile_L1
	#doxygen Doxyfile_L2
	#doxygen Doxyfile_L3
	mkdir -p .stamp
	touch .stamp/doxygen

DOXYREST_FRAME = $(dir $(shell which doxyrest))/../doxyrest_frame

.stamp/doxyrest: .stamp/doxygen
	rm -rf src/rst_L1 src/rst_L2 src/rst_L3
	mkdir src/rst_L1 src/rst_L2 src/rst_L3
	doxyrest xml_L1/index.xml -o src/rst_L1/index.rst \
		-F $(DOXYREST_FRAME) -f c_index.rst.in -Dg_verbatimToCodeBlock
	#doxyrest xml_L2/index.xml -o src/rst_L2/index.rst \
	#	-F $(DOXYREST_FRAME) -f c_index.rst.in -Dg_verbatimToCodeBlock
	#doxyrest xml_L3/index.xml -o src/rst_L3/index.rst \
	#	-F $(DOXYREST_FRAME) -f c_index.rst.in -Dg_verbatimToCodeBlock
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
	rm -rf .stamp xml_L1 xml_L2 xml_L3 src/rst_L1 src/rst_L2 src/rst_L3 build/html build/doctrees
