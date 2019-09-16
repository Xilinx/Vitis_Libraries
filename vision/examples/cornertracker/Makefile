#***************************************************************************
#	Copyright (c) 2019, Xilinx, Inc.
#	All rights reserved.

#	Redistribution and use in source and binary forms, with or without modification, 
#	are permitted provided that the following conditions are met:

#	1. Redistributions of source code must retain the above copyright notice, 
#	this list of conditions and the following disclaimer.

#	2. Redistributions in binary form must reproduce the above copyright notice, 
#	this list of conditions and the following disclaimer in the documentation 
#	and/or other materials provided with the distribution.

#	3. Neither the name of the copyright holder nor the names of its contributors 
#	may be used to endorse or promote products derived from this software 
#	without specific prior written permission.

#	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
#	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
#	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
#	IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
#	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
#	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
#	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
#	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
#	EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#***************************************************************************/



#-------------------------------------------------------------------------------
# Board specific variables
#-------------------------------------------------------------------------------
PLATFORM 	= zcu102 #zcu104
CLK_VAL        ?= 0 
DMCLK_VAL      ?= ${CLK_VAL}
#PLATFORM_T     ?= ZCU102-RV

#-------------------------------------------------------------------------------
# Enables for Lib generation
#-------------------------------------------------------------------------------
LIB_GEN_ENA    ?= 0

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Function related variables
#-------------------------------------------------------------------------------
# Note: Valid values for FUNC_TYPE are "imgproc / core / features"
FUNC_TYPE        := imgproc
FUNC_TYPE2		 := video
FUNC_TYPE4       := features
FUNC_NAME        := xf_corner_tracker
FUNC_NAME_1      := xf_pyr_down
FUNC_NAME_2      := xf_pyr_dense_optical_flow_wrapper
FUNC_NAME_3      := xf_corner_update
FUNC_NAME_4      := xf_harris
FUNC_NAME_5      := xf_corner_img_to_list

FUNC_DIR       := cornertracker
HW_TOP_NAME    := "xf::pyrDown<0,1080,1920,1,0>"
HW_TOP_NAME_1    := "xf::densePyrOpticalFlow<5,50,11,16,10,0,1080,1920,1,0>"
HW_TOP_NAME_2    := "xf::cornerUpdate<10000,3,1080,1920,1>"
HW_TOP_NAME_3    := "xf::cornerHarris<3,3,1,0,1080,1920,1,0>"
HW_TOP_NAME_4    := "xf::cornersImgToList<10000,0,1080,1920,1>"
EXECUTABLE     := cornerTracker.elf

#-------------------------------------------------------------------------------
# Internal Path variables
#-------------------------------------------------------------------------------
XLNX_SRC_PATH  ?= $(shell pwd)/../..
WORK_PATH      := $(shell pwd)
PROJ_TOP_PATH  := ${XLNX_SRC_PATH}
TB_SRC_PATH    := ${PROJ_TOP_PATH}/examples/${FUNC_DIR}
SRC_PATH       := ${PROJ_TOP_PATH}/include/${FUNC_TYPE}
SRC_PATH1       := ${PROJ_TOP_PATH}/include/${FUNC_TYPE2}
SRC_PATH4      := ${PROJ_TOP_PATH}/include/${FUNC_TYPE4}

#-------------------------------------------------------------------------------
# Source files
#-------------------------------------------------------------------------------
APPSOURCES := ${TB_SRC_PATH}/${FUNC_NAME}_tb.cpp ${TB_SRC_PATH}/${FUNC_NAME}_accel.cpp

#-------------------------------------------------------------------------------
# Library/Include paths
#-------------------------------------------------------------------------------
IDIRS = -I. -I<path-to-arm-opencv-includes> -I${PROJ_TOP_PATH}/include
LDIRS = -L<path-to-arm-opencv-libs>
LLIBS ?= -lopencv_imgcodecs -lopencv_core -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann

#-------------------------------------------------------------------------------
# sds++/c++ Flags
#-------------------------------------------------------------------------------
DEBUG_FLAG  :=#-ggdb
VERBOSE     :=#-verbose
OLEVEL      := -O0
WARN_FLAGS  := -Wall -Wno-unused-label -Wno-unused-variable -Wno-unused-function# -Wno-int-in-bool-context -Wno-mismatched-tags -Wno-unknown-warning-option -Wno-unneeded-internal-declaration -Wno-extra-tokens

SDSCC_FLAGS := -sds-pf ${PLATFORM} \
               -sds-hw ${HW_TOP_NAME} ${TB_SRC_PATH}/${FUNC_NAME}_accel.cpp -files ${SRC_PATH}/${FUNC_NAME_1}.hpp -clkid ${CLK_VAL} -sds-end \
			   -sds-hw ${HW_TOP_NAME_1} ${TB_SRC_PATH}/${FUNC_NAME}_accel.cpp -files ${SRC_PATH1}/${FUNC_NAME_2}.hpp -clkid ${CLK_VAL} -sds-end \
			   -sds-hw ${HW_TOP_NAME_2} ${TB_SRC_PATH}/${FUNC_NAME}_accel.cpp -files ${SRC_PATH}/${FUNC_NAME_3}.hpp -clkid ${CLK_VAL} -sds-end \
			   -sds-hw ${HW_TOP_NAME_3} ${TB_SRC_PATH}/${FUNC_NAME}_accel.cpp -files ${SRC_PATH4}/${FUNC_NAME_4}.hpp -clkid ${CLK_VAL} -sds-end \
			   -sds-hw ${HW_TOP_NAME_4} ${TB_SRC_PATH}/${FUNC_NAME}_accel.cpp -files ${SRC_PATH}/${FUNC_NAME_5}.hpp -clkid ${CLK_VAL} -sds-end \
			   -dmclkid ${DMCLK_VAL}


CFLAGS := ${DEBUG_FLAG} ${IDIRS} ${OLEVEL} ${WARN_FLAGS} -DHLS_NO_XIL_FPO_LIB -hls-target 1 -mstrict-align 
LFLAGS := ${DEBUG_FLAG} ${LDIRS} ${LLIBS} 
 
ifeq (${LIB_GEN_ENA}, 1)
  CFLAGS += -FPIC
  LFLAGS += -shared
endif

#-------------------------------------------------------------------------------
# Tool variables
#-------------------------------------------------------------------------------
CC := sds++

#-------------------------------------------------------------------------------
# Internal variables
#-------------------------------------------------------------------------------
SRC_FILES := $(notdir $(APPSOURCES))
OBJECTS   := ${SRC_FILES:%.cpp=${WORK_PATH}/%.o}

#-------------------------------------------------------------------------------
# Target(s) declaration:
#-------------------------------------------------------------------------------
# Default
# ------------------------
.PHONY: all
all: ${EXECUTABLE}

.PHONY: obj
obj: ${WORK_PATH}/${FUNC_NAME}_accel.o

# Linking
# ------------------------
${EXECUTABLE}: ${OBJECTS}
	${CC} ${SDSCC_FLAGS} ${OBJECTS} ${LFLAGS} -o $@

# Dependencies
# Note: Include only if exists
# ------------------------
#-include ${DEPS}

# Compiling
# ------------------------
${WORK_PATH}/%.o: ${TB_SRC_PATH}/%.cpp
	${CC} ${SDSCC_FLAGS} -c  ${CFLAGS} $< -o $@

# House keeping
# ------------------------
clean:
	${RM} ${EXECUTABLE} ${OBJECTS}

ultraclean: clean
	${RM} -rf _sds sd_card ${EXECUTABLE}.bit ${EXECUTABLE} ${OBJECTS} *.s

.PHONY: copy
copy:
	@echo "Makefile got changed, so this target (copy) is obsolete."
