#!/usr/bin/env python

import sys
from sys import argv
import json
import glob
import os
import re
import subprocess
import fileinput
sys.dont_write_bytecode = True

#pwd = os.path.dirname(sys.path[0])
#script, folder = argv
os.system("mkdir -p temp common/thirdParty/")
os.system("mkdir -p L1/include/hw/")
os.system("mkdir -p L2/src/hw/ L2/test/lz4/src/")

os.system("mkdir -p L2/test/lz4/src/")
os.system("mkdir -p L2/test/snappy/src/")

os.chdir("temp/")
os.system("git clone https://gitenterprise.xilinx.com/hatchuta/xil_lz4.git -b new_lib_struct")
os.chdir("../")
pwd = "."

lz4_kernel_dir = "L2/src/hw/"
xil_lz4_folder = "temp/xil_lz4/"

#lz4
lz4_host_dir = "L2/test/lz4/"
#snappy
snappy_host_dir = "L2/test/snappy/"

# Copy script begins

os.system("cp -rf " + xil_lz4_folder +
"/utility/ " + pwd + "/common/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/ " + pwd + "/common/")
os.system("cp -rf " + xil_lz4_folder +
"/LICENSE.md " + pwd)
os.system("cp -rf " + xil_lz4_folder +
"/README.md " + pwd)
os.system("cp -rf ./temp/xil_lz4/utility/makefile_gen ./common/utility/")
#os.system("cp -rf " + xil_lz4_folder +
#"/lz4_stream_ip/ " + pwd)
#os.system("cp -rf " + xil_lz4_folder +
#"/xil_lz4/ " + pwd)
#os.system("cp -rf " + xil_lz4_folder +
#"/xil_lz4/Makefile " + lz4_host_dir)

####### LZ4 copy #####################
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/README.md " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/reports " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/xxhash " + "./common/thirdParty/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/scripts " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/FAQ.md " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/LICENSE.md " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/*.list " + "L2/test/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/lz4 " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/utils.mk " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/config.mk " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/src/defns.h " + lz4_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/src/xil_lz4.cpp " + lz4_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/src/xil_lz4.h " + lz4_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/src/xil_lz4_config.h " + lz4_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/src/xil_lz4_compress_kernel.cpp " + lz4_kernel_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_lz4/src/xil_lz4_decompress_kernel.cpp " + lz4_kernel_dir)
os.system("cp -rf " + xil_lz4_folder +
"xil_lz4/description.json " + lz4_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"xil_lz4/config.mk " + lz4_host_dir)

###### Snappy copy ##################
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/README.md " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/reports " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/scripts " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/FAQ.md " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/LICENSE.md " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/utils.mk " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/config.mk " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/src/defns.h " + snappy_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/src/xil_snappy.cpp " + snappy_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/src/xil_snappy.h " + snappy_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/src/xil_snappy_config.h " + snappy_host_dir + "src/")
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/src/xil_snappy_compress_kernel.cpp " + lz4_kernel_dir)
os.system("cp -rf " + xil_lz4_folder +
"/xil_snappy/src/xil_snappy_decompress_kernel.cpp " + lz4_kernel_dir)
os.system("cp -rf " + xil_lz4_folder +
"xil_snappy/description.json " + snappy_host_dir)
os.system("cp -rf " + xil_lz4_folder +
"xil_snappy/config.mk " + snappy_host_dir)

#####################################

os.system("cp -rf " + xil_lz4_folder +
"/libs/stream_utils/mm2s.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/stream_utils/stream_downsizer.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/stream_utils/stream_upsizer.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/stream_utils/s2mm.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/lz_compress/lz_compression.h " + "L1/include/hw/lz_compress.h")
os.system("cp -rf " + xil_lz4_folder +
"/libs/lz_compress/lz_decompress.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/lz_compress/lz_optional.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/lz_compress/lz4_compress.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/lz_compress/lz4_decompress.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/lz_compress/snappy_compress.h " + "L1/include/hw/")
os.system("cp -rf " + xil_lz4_folder +
"/libs/lz_compress/snappy_decompress.h " + "L1/include/hw/")

os.system("mv " + xil_lz4_folder + "/benchmark " + "common/")
os.system("mv " + xil_lz4_folder + "/img " + "common/")
#os.system("mv " + xil_lz4_folder + "/libs " + "common/")
#os.system("mv " + xil_lz4_folder + "/lz4_stream_ip " + "common/")
#os.system("mv " + xil_lz4_folder + "/utility " + "common/")

stream_libs = ["stream_utils_HDRS:=${COMMON_REPO}/L1/include/hw/mm2s.h","stream_utils_HDRS:=${COMMON_REPO}/L1/include/hw/stream_upsizer.h","stream_utils_HDRS:=${COMMON_REPO}/L1/include/hw/stream_downsizer.h","stream_utils_HDRS:=${COMMON_REPO}/L1/include/hw/s2mm.h","stream_utils_CLFLAGS:=-I${COMMON_REPO}/L1/include/hw/"]
lz_libs = ["lz_utils_HDRS:=${COMMON_REPO}/L1/include/hw/lz_compress.h","lz_utils_HDRS:=${COMMON_REPO}/L1/include/hw/lz_decompress.h","lz_utils_HDRS:=${COMMON_REPO}/L1/include/hw/lz_optional.h","lz_utils_HDRS:=${COMMON_REPO}/L1/include/hw/lz4_compress.h","lz_utils_HDRS:=${COMMON_REPO}/L1/include/hw/lz4_decompress.h","lz_utils_CLFLAGS:=-I${COMMON_REPO}/L1/include/hw/"]

stream_file = open("L1/include/stream_utils.mk","w+")
lz_file = open("L1/include/lz_utils.mk","w+")

for i in range(0, len(stream_libs)):
    stream_file.write(stream_libs[i])
    stream_file.write("\n")

for i in range(0, len(lz_libs)):
    lz_file.write(lz_libs[i])
    lz_file.write("\n")

stream_file.close()
lz_file.close()

#os.system("cp -rf L2/* L1/")
#os.system("rm -rf L2/include")


cantrbry = open("./L2/test/cantrbry.list","r")
cantrbry_data = cantrbry.read()
cantrbry.close()
large_canter = open("./L2/test/largecanter.list","r")
large_canter_data = large_canter.read()
large_canter.close()
silesia = open("./L2/test/silesia.list","r")
silesia_data = silesia.read()
silesia.close()
calgarycorpus = open("./L2/test/calgarycorpus.list","r")
calgarycorpus_data = calgarycorpus.read()
calgarycorpus.close()
test = open("./L2/test/test.list","r")
test_data = test.read()
test.close()

cdata = cantrbry_data.replace("../benchmark","../../../common/benchmark")
lcdata = large_canter_data.replace("../benchmark","../../../common/benchmark")
sdata = silesia_data.replace("../benchmark","../../../common/benchmark")
ccdata = calgarycorpus_data.replace("../benchmark","../../../common/benchmark")
tdata = test_data.replace("../benchmark","../../../common/benchmark")

cantrbry = open("./L2/test/cantrbry.list","w")
cantrbry.write(cdata)
cantrbry.close()
large_canter = open("./L2/test/largecanter.list","w")
large_canter.write(lcdata)
large_canter.close()
silesia = open("./L2/test/silesia.list","w")
silesia.write(sdata)
silesia.close()
calgarycorpus = open("./L2/test/calgarycorpus.list","w")
calgarycorpus.write(ccdata)
calgarycorpus.close()
test = open("./L2/test/test.list","w")
test.write(tdata)
test.close()


cmdparser = open("./common/libs/cmdparser/cmdparser.mk","r")
cmdparser_data = cmdparser.read()
cmdparser.close()
logger = open("./common/libs/logger/logger.mk","r")
logger_data = logger.read()
logger.close()
lz_comp = open("./common/libs/lz_compress/lz_compress.mk","r")
lz_comp_data = lz_comp.read()
lz_comp.close()
opencl = open("./common/libs/opencl/opencl.mk","r")
opencl_data = opencl.read()
opencl.close()
stream_utils = open("./common/libs/stream_utils/stream_utils.mk","r")
stream_utils_data = stream_utils.read()
stream_utils.close()
xcl = open("./common/libs/xcl2/xcl2.mk","r")
xcl_data = xcl.read()
xcl.close()

cmddata = cmdparser_data.replace("${COMMON_REPO}/libs","${COMMON_REPO}/common/libs")
ldata  = logger_data.replace("${COMMON_REPO}/libs","${COMMON_REPO}/common/libs")
lzdata = lz_comp_data.replace("${COMMON_REPO}/libs","${COMMON_REPO}/common/libs")
odata = opencl_data.replace("${COMMON_REPO}/libs","${COMMON_REPO}/common/libs")
sdata = stream_utils_data.replace("${COMMON_REPO}/libs","${COMMON_REPO}/common/libs")
xdata = xcl_data.replace("${COMMON_REPO}/libs","${COMMON_REPO}/common/libs")

cmdparser = open("./common/libs/cmdparser/cmdparser.mk","w")
cmdparser.write(cmddata)
cmdparser.close()
logger = open("./common/libs/logger/logger.mk","w")
logger.write(ldata)
logger.close()
lz_comp = open("./common/libs/lz_compress/lz_compress.mk","w")
lz_comp.write(lzdata)
lz_comp.close()
opencl = open("./common/libs/opencl/opencl.mk","w")
opencl.write(odata)
opencl.close()
stream_utils = open("./common/libs/stream_utils/stream_utils.mk","w")
stream_utils.write(sdata)
stream_utils.close()
xcl = open("./common/libs/xcl2/xcl2.mk","w")
xcl.write(xdata)
xcl.close()

readme = open("./L2/test/lz4/README.md","r")
readme_data = readme.read()
readme.close()

rdata = readme_data.replace("../img/lzx_comp.png","../../../common/img/lzx_comp.png")
rdata = rdata.replace("../img/lzx_decomp.png","../../../common/img/lzx_decomp.png")

readme = open("./L2/test/lz4/README.md","w")
readme.write(rdata)
readme.close()

readme = open("./L2/test/snappy/README.md","r")
readme_data = readme.read()
readme.close()

rdata = readme_data.replace("../img/lzx_comp.png","../../../common/img/lzx_comp.png")
rdata = rdata.replace("../img/lzx_decomp.png","../../../common/img/lzx_decomp.png")

readme = open("./L2/test/snappy/README.md","w")
readme.write(rdata)
readme.close()

readme = open("README.md","r")
readme_data = readme.read()
readme.close()

rdata = readme_data.replace("./img/lzx_comp.png","./common/img/lzx_comp.png")
rdata = rdata.replace("./img/lzx_decomp.png","./common/img/lzx_decomp.png")

readme = open("README.md","w")
readme.write(rdata)
readme.close()

utils_mk = open("./L2/test/lz4/utils.mk","r")
utils_data = utils_mk.read()
utils_mk.close()

udata = utils_data.replace("$(COMMON_REPO)/utility","$(COMMON_REPO)/common/utility/")

utils_mk = open("./L2/test/lz4/utils.mk","w")
utils_mk.write(udata)
utils_mk.close()

utils_mk = open("./L2/test/snappy/utils.mk","r")
utils_data = utils_mk.read()
utils_mk.close()

udata = utils_data.replace("$(COMMON_REPO)/utility","$(COMMON_REPO)/common/utility/")

utils_mk = open("./L2/test/snappy/utils.mk","w")
utils_mk.write(udata)
utils_mk.close()

os.system("rm -rf common/libs/stream_utils")
os.system("rm -rf common/libs/lz_compress")
os.system("rm -rf temp/")


#####  L2 changes ###############

#os.system("mkdir -p L2/demos")
#os.system("cp -rf L2/demos/ L2/")


#os.system(pwd + "/common/utility/makefile_gen/update_makegen_all.sh")
#os.system(pwd + "/common/utility/readme_gen/update_all_readme.sh")
#os.system(pwd + "/common/utility/check_readme.sh")
#os.system(pwd + "/common/utility/check_makefile.sh")

# Since the new Makefile structute has changed we need to copy some files like
# config.mk in rtl_kernel examples from Scout present repo.
