#!/usr/bin/env python
from sys import argv
import json
import glob
import os
import re
import subprocess

def create_params(target,data):    
    target.write("# Points to Utility Directory\n")
    dirName = os.getcwd()
    dirNameList = list(dirName.split("/"))
    dirNameIndex = dirNameList.index("L2")
    diff = len(dirNameList) - dirNameIndex
    target.write("COMMON_REPO = ")
    while diff > 0:
	target.write("../")
	diff -= 1 
    target.write("\n")
    target.write("ABS_COMMON_REPO = $(shell readlink -f $(COMMON_REPO))\n")
    target.write("\n")
    target.write("TARGET := hw\n")
    target.write("\n")
    target.write("include ./utils.mk\n")
    target.write("include ./config.mk\n")
    target.write("\n")
    target.write("XSA := $(call device2sanxsa, $(DEVICE))\n")
    target.write("TEMP_DIR := ./_x_$(TARGET)_$(XSA)\n")
    target.write("BUILD_DIR := ./build_dir_$(TARGET)_$(XSA)\n")
    target.write("\n")
    target.write("CXX := ")
    target.write("$(XILINX_VITIS)/bin/xcpp\n")
    target.write("XOCC := ")
    target.write("$(XILINX_VITIS)/bin/xocc\n")
    target.write("\n")
    #add_libs1(target, data)
    add_libs2(target, data) 
    target.write("\n")
    return

def add_libs1(target, data):
    target.write("\n#Include Libraries\n")
    #target.write("include $(ABS_COMMON_REPO)/common/libs/opencl/opencl.mk\n")
    if "includes" in data:
        for lib in data["includes"]:
            path = lib["location"]
            path = path.replace('GIT_REPO', '$(ABS_COMMON_REPO)')
            target.write("include " + path)
            target.write(lib["name"])
            target.write(".mk")
            target.write("\n")
    return

def add_libs2(target, data):
    target.write("#Host and Common includes\n")
    target.write("HOST_CXXFLAGS+=-I./src/\n")
    target.write("HOST_CXXFLAGS+=-I$(XILINX_XRT)/include/\n")
    if "include_paths" in data:
        for lib in data["include_paths"]:
            target.write("HOST_CXXFLAGS +=-I")
            lib = lib.replace("GIT_REPO","${ABS_COMMON_REPO}")
            target.write(lib)
            target.write("\n")
    ''''
    if "libs" in data:
        target.write("CXXFLAGS +=")
        for lib in data["libs"]:
            if(lib != "stream_utils" and lib != "lz_utils"):
                target.write(" $(")
                target.write(lib)
                target.write("_CXXFLAGS)")
        target.write("\n")
        target.write("HOST_SRCS +=")
        for lib in data["libs"]:
            if(lib != "stream_utils" and lib != "lz_utils"  and lib != "opencl"):
                target.write(" $(")
                target.write(lib)
                target.write("_SRCS)")
        target.write("\n")
        target.write("HOST_HDRS +=")
        for lib in data["libs"]:
            target.write(" $(")
            target.write(lib)
            target.write("_HDRS)")
    target.write("\n")
    '''
    
    '''
    if "linker" in data:
        if "libraries" in data["linker"]:
            target.write("\nCXXFLAGS +=")
            for lin in data["linker"]["libraries"]:
                target.write(" ")
                target.write("-l")
                target.write(lin)
        if "options" in data["linker"]:
            for lin in data["linker"]["options"]:
  	        target.write(" ")
	        target.write(lin)
        target.write("\n")                
    '''
    return

def add_host_includes(target, data):
    if "host_srcs" in data:
        target.write("#Host and Common sources\n")
        for src in data["host_srcs"]:
            target.write("HOST_SRCS += ")
            src = src.replace('GIT_REPO', '$(ABS_COMMON_REPO)')
    	    target.write(src)
            target.write("\n")
        target.write("\n")
    if "host_hdrs" in data:
        target.write("#Host and Common headers\n")
        for hdr in data["host_hdrs"]:
            target.write("HOST_HDRS += ")
            hdr = hdr.replace('GIT_REPO', '$(ABS_COMMON_REPO)')
            target.write(hdr)
            target.write("\n")

    target.write("\n")

    return

def add_host_flags(target, data):
    target.write("# Host compiler global settings\n")
    target.write("HOST_CXXFLAGS += ")
    target.write("-fmessage-length=0")
        
    if "compiler" in data:
	if "options" in data["compiler"]:
	    target.write(data["compiler"]["options"])
    target.write("\n")
    target.write("LDFLAGS=-L$(XILINX_XRT)/lib/ -lOpenCL -pthread\n")
    target.write("LDFLAGS += ")
    target.write("-lrt -Wno-unused-label -Wno-narrowing -std=c++0x -DVERBOSE")

    target.write("\n")

    return
def add_kernel_flags(target, data):
    target.write("# Kernel compiler global settings\n")
    target.write("CLFLAGS += ")
    target.write("-t $(TARGET) --platform $(DEVICE) --save-temps \n")   

    if "containers" in data:
        for con in data["containers"]:
            for acc in con["accelerators"]:
                if "max_memory_ports" in acc:
		    target.write("CLFLAGS += ")
                    target.write(" --max_memory_ports ")
                    target.write(acc["kernel_name"])
        	    target.write("\n")

    if "containers" in data:
        for con in data["containers"]:
            if "clflags" in con:
                target.write("CLFLAGS +=")
                flags = con["clflags"].split(" ")
                for flg in flags[0:]:
                    target.write(" ")
                    flg = flg.replace('PROJECT', '.')
                    target.write(flg)
        target.write("\n")
    #target.write("CLFLAGS += $(stream_utils_CLFLAGS) $(lz_utils_CLFLAGS)")                
    
    if "compiler" in data:
        if "symbols" in data["compiler"]:
            target.write("\nCXXFLAGS +=")
            for sym in data["compiler"]["symbols"]:
                target.write(" ")
                target.write("-D")
                target.write(sym)
        target.write("\n")

    '''
    if "containers" in data:
        for con in data["containers"]:
            if  "ldclflags" in con:
		target.write("\n")
                target.write("# Kernel linker flags\n")
                target.write("LDCLFLAGS +=")
		ldclflags = con["ldclflags"].split(" ")
		for flg in ldclflags[0:]:
		    target.write(" ")
		    flg = flg.replace('PROJECT', '.')
		    target.write(flg)
            target.write("\n")
    target.write("\n")
    '''
    target.write("\n")

    return

def add_containers(target, data):
    if "containers" in data:
	for con in data["containers"]:
	    target.write("BINARY_CONTAINERS += $(BUILD_DIR)/")
            target.write(con["name"])
            target.write(".xclbin\n")
	    if "accelerators" in con:
		for acc in con["accelerators"]:
		    target.write("BINARY_CONTAINER_")
                    target.write(con["name"])
                    target.write("_OBJS += $(TEMP_DIR)/") 
		    target.write(acc["kernel_name"])
                    target.write(".xo\n")       	
    target.write("\n")

def building_kernel(target, data):
    if "containers" in data:
        target.write("\n# Binary creation\n")
        for con in data["containers"]:
            target.write("$(BUILD_DIR)/")
            target.write(con["name"])
            target.write(".xclbin: ")
            if "accelerators" in con:
                for acc in con["accelerators"]:
                    target.write("$(TEMP_DIR)/")
                    target.write(acc["kernel_name"])
                    target.write(".xo ")
            target.write("\n")
            target.write("\tmkdir -p $(BUILD_DIR)\n")
            target.write("\t$(XOCC) $(CLFLAGS) --temp_dir ")
            target.write("$(BUILD_DIR)/"+ con["name"] )
            target.write(" --report_dir reports/$(BUILD_DIR)/"+ con["name"] )
            target.write(" -l ")
            if "ldclflags" in con:
                target.write(con["ldclflags"])
            for acc in con["accelerators"]:
                target.write(" --nk ")
                target.write(acc["kernel_name"])
                if "num_compute_units" in acc.keys():
		    target.write(":")
		    target.write(acc["num_compute_units"])
		else:
		    target.write(":1")
            target.write(" -o'$@' $(+)\n\n")
    target.write("\n")

    target.write("# Building kernel\n")
    if "containers" in data:
        for con in data["containers"]:
            if "accelerators" in con:
                for acc in con["accelerators"]:
                    target.write("$(TEMP_DIR)/")
                    target.write(acc["kernel_name"])
                    target.write(".xo: ")
                    if "srcs" in acc:
                        for src in acc["srcs"]:
                            target.write(src)
                        target.write(" ")                          
                    target.write("\n")
                    target.write("\tmkdir -p $(TEMP_DIR)\n")
                    target.write("\t$(XOCC) $(CLFLAGS) ")
                    if "include_path" in acc:
                        for path in acc["include_path"]:
                            path = path.replace("GIT_REPO","$(ABS_COMMON_REPO)")
                            target.write("-I" + path)
                            target.write(" ")                            
                    target.write("--temp_dir $(TEMP_DIR)/"+ con["name"])
                    target.write(" --report_dir reports/$(TEMP_DIR)/"+ con["name"])
                    target.write(" -c -k ")
                    target.write(acc["kernel_name"])
                    target.write(" ")
                    if "clflags" in acc:
                        target.write(acc["clflags"])
                    target.write(" -I'$(<D)'")
                    target.write(" -o'$@' '$<'\n")
    target.write("\n\n")
    return

def building_kernel_rtl(target, data):
    target.write("# Building kernel\n")
    if "containers" in data:
	for con in data["containers"]:
	    target.write("$(BUILD_DIR)/")
            target.write(con["name"])
            target.write(".xclbin:")
	    target.write(" $(BINARY_CONTAINER_")
            target.write(con["name"])
            target.write("_OBJS)\n")
	    target.write("\tmkdir -p $(BUILD_DIR)\n")
	    target.write("\t$(XOCC) $(CLFLAGS) $(LDCLFLAGS) -l")
	    target.write(" $(BUILD_DIR)/")
	    target.write(con["name"])
            for acc in con["accelerators"]:
                target.write(" $(BUILD_DIR)/")
		target.write(acc["kernel_name"])
	    target.write("\n\n")
    return

def building_host(target, data):
    target.write("\n #Building Host\n")
    target.write("$(EXECUTABLE): $(HOST_SRCS)\n")
    target.write("\t$(CXX) $(HOST_CXXFLAGS) $(HOST_SRCS) -o '$@' $(LDFLAGS)\n")
    target.write("\n")
    target.write("#emconfig.json generation\n")
    target.write("emconfig:$(EMCONFIG_DIR)/emconfig.json\n")
    target.write("$(EMCONFIG_DIR)/emconfig.json:\n")
    target.write("\temconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)")
    if "num_devices" in data:
        target.write(" --nd ")
        target.write(data["num_devices"])
    target.write("\n\n")        
    return

def building_host_rtl(target, data):
    target.write("# Building Host\n")
    target.write("$(EXECUTABLE): check-xrt $(HOST_SRCS) $(HOST_HDRS)\n")
    target.write("\t$(CXX) $(CXXFLAGS) $(HOST_SRCS) $(HOST_HDRS) -o '$@' $(LDFLAGS)\n")
    target.write("\n")
    target.write("emconfig:emconfig.json\n")
    target.write("emconfig.json:\n")
    target.write("\temconfigutil --platform $(XSA) --nd 1\n\n")
    return

def profile_report(target):
    target.write("[Debug]\n")
    target.write("profile=true\n")

    return

def mk_clean(target, data):
    target.write("# Cleaning stuff\n")
    target.write("clean:\n")
    target.write("\t-$(RMDIR) $(EXECUTABLE)\n")
    target.write("\t-$(RMDIR) vitis_* TempConfig system_estimate.xtxt *.rpt .run/\n")
    target.write("\t-$(RMDIR) src/*.ll _xocc_* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb\n")
    target.write("\n")

    target.write("cleanall: clean\n")
    target.write("\t-$(RMDIR) build_dir*\n")
    target.write("\t-$(RMDIR) _x*\n")
    target.write("\t-$(RMDIR) $(ABS_COMMON_REPO)/common/benchmark/cantrbry/*.xe2xd* $(ABS_COMMON_REPO)/common/benchmark/silesia/*.xe2xd*\n")
    if "output_files" in data:         
        target.write("\t-$(RMDIR) ")
        args = data["output_files"].split(" ")
        for arg in args[0:]:
            target.write("./")
            target.write(arg)
            target.write(" ")       
    target.write("\n")

    return

def mk_build_all(target, data):

    args = []
    if "cmd_args" in data:
        args = data["cmd_args"].split(" ")
        if any("/data" in string for string in args):
            target.write("DATA = ./data\n")
    
    target.write("#Host Executable\n")
    target.write("EXECUTABLE = ")
    if "host_exe" in data:
        target.write(data["host_exe"])    
    else: 
        target.write("host")

    target.write("\n\n#CommandLine arguments")
    if "cmd_args" in data:
    	target.write("\n")
        target.write("CMD_ARGS =")
	cmd_args = data["cmd_args"].split(" ")
	for cmdargs in cmd_args[0:]:
	    target.write(" ")
            cmdargs = cmdargs.replace('.xclbin', '')
            cmdargs = cmdargs.replace('BUILD', '$(BUILD_DIR)')
            cmdargs = cmdargs.replace('PROJECT', '.')
	    target.write(cmdargs)
    	    if "$(XCLBIN)" in cmdargs:
            	target.write(".$(TARGET).$(XSA).xclbin")
    target.write("\n\n")

    target.write("CP = cp -rf\n")
    target.write("EMCONFIG_DIR = $(TEMP_DIR)")
    target.write("\n\n")


    target.write(".PHONY: all clean cleanall docs emconfig\n")
    target.write("all: $(EXECUTABLE) ")
    if "containers" in data:
        for con in data["containers"]:
            target.write("$(BUILD_DIR)/")
            target.write(con["name"])
            target.write(".xclbin ")
    target.write("emconfig\n")
    target.write("\n")
    
    target.write(".PHONY: exe\n")
    target.write("exe: $(EXECUTABLE)\n")
    target.write("\n")
    '''    
    target.write(".PHONY: build\n")
    target.write("build: $(BINARY_CONTAINERS)\n")
    target.write("\n")
    '''
    target.write("#Run application\n")
    mk_check(target, data)
    counter = 0
    if "containers" in data:
	for con in data["containers"]:
	    if "accelerators" in con:
		for acc in con["accelerators"]:
		    if "kernel_type" in acc:
		    	if acc["kernel_type"] == "RTL":
			    counter = 1
    if counter == 1:
        add_kernel_flags(target, data)
        building_kernel_rtl(target, data)
    else:
        add_kernel_flags(target, data)
        building_kernel(target, data)
    return

def mk_check(target, data):
    target.write("check: all\n")
    if "nboard" in data:
        for board in data["nboard"]:
            target.write("ifeq ($(findstring ")
	    target.write(board)
	    target.write(", $(DEVICE)), ")
	    target.write(board)
            target.write(")\n")                   
            target.write("$(error Nothing to be done for make)\n")
            target.write("endif\n")
        target.write("\n")
    target.write("ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))\n")
    target.write("\t$(CP) $(EMCONFIG_DIR)/emconfig.json .\n") 
    target.write("\tXCL_EMULATION_MODE=$(TARGET) ./$(EXECUTABLE) ")
    
    if "cmd_args" in data:
        args = data["cmd_args"].split(" ")    
        for arg in args[1:]:
            arg = arg.replace('BUILD', '$(BUILD_DIR)')
            target.write(arg)
            target.write(" ")
    target.write("\nendif\n")
    '''
	    arg = arg.replace('.xclbin', '')
            arg = arg.replace('BUILD', '$(XCLBIN)')
	    arg = arg.replace('PROJECT', '.')
	    target.write(arg)
  	    if "$(XCLBIN)" in arg:
	    	target.write(".$(TARGET).$(XSA).xclbin")
    target.write("\nelse\n")        
    target.write("\t ./$(EXECUTABLE)")
    if "cmd_args" in data:
        args = data["cmd_args"].split(" ")    
        for arg in args[0:]:
            target.write(" ")
	    arg = arg.replace('.xclbin', '')
	    arg = arg.replace('BUILD', '$(XCLBIN)')
	    arg = arg.replace('PROJECT', '.')
	    target.write(arg)
	    if "$(XCLBIN)" in arg:
            	target.write(".$(TARGET).$(XSA).xclbin")
    target.write("\nendif\n")
    '''
    if "targets" in data:
        target.write("ifneq ($(TARGET),$(findstring $(TARGET),")
        args = data["targets"]
        for arg in args:
            target.write(" ")
            target.write(arg)
        target.write("))\n")
        target.write("$(warning WARNING:Application supports only")
        for arg in args:
            target.write(" ")
            target.write(arg)
        target.write(" TARGET. Please use the target for running the application)\n")
        target.write("endif\n")
        target.write("\n")

    #if data["example"] != "00 Matrix Multiplication":
	#target.write("\tvitis_analyze profile -i profile_summary.csv -f html\n")
    target.write("\n")

def run_nimbix(target, data):
    target.write("run_nimbix: all\n")
    if "cmd_args" in data:
    	target.write("\t$(COMMON_REPO)/utility/nimbix/run_nimbix.py $(EXECUTABLE) $(CMD_ARGS) $(XSA)\n\n")
    else:
    	target.write("\t$(COMMON_REPO)/utility/nimbix/run_nimbix.py $(EXECUTABLE) $(XSA)\n\n")	
    
def aws_build(target):
    target.write("aws_build: check-aws_repo $(BINARY_CONTAINERS)\n")
    target.write("\t$(COMMON_REPO)/utility/aws/run_aws.py $(BINARY_CONTAINERS)\n\n")

def mk_help(target):
    target.write(".PHONY: help\n")
    target.write("\n")
    target.write("help::\n")
    target.write("\t$(ECHO) \"Makefile Usage:\"\n")
    target.write("\t$(ECHO) \"  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>\"\n");
    target.write("\t$(ECHO) \"      Command to generate the design for specified Target and Device.\"\n")
    target.write("\t$(ECHO) \"\"\n")
    target.write("\t$(ECHO) \"  make clean \"\n");
    target.write("\t$(ECHO) \"      Command to remove the generated non-hardware files.\"\n")
    target.write("\t$(ECHO) \"\"\n")
    target.write("\t$(ECHO) \"  make cleanall\"\n")
    target.write("\t$(ECHO) \"      Command to remove all the generated files.\"\n")
    target.write("\t$(ECHO) \"\"\n")
    target.write("\t$(ECHO) \"  make check TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform>\"\n");
    target.write("\t$(ECHO) \"      Command to run application in emulation.\"\n")
    target.write("\t$(ECHO) \"\"\n")
    target.write("\n")

def create_mk(target, data):
    mk_help(target)
    create_params(target,data)
    mk_build_all(target, data)
    add_host_includes(target, data)
    add_host_flags(target, data)
    building_host(target, data)
    #add_containers(target, data)
    #run_nimbix(target, data)
    #aws_build(target)
    mk_clean(target,data)
    return 

def create_utils(target):
    dirName = os.getcwd()
    dirNameList = list(dirName.split("/"))
    dirNameIndex = dirNameList.index("apps")
    diff = len(dirNameList) - dirNameIndex - 1
    while diff > 0:
	    os.chdir('..')
	    diff -= 1
    os.chdir("utility")
    source = open("utils.mk", "r")
    data = source.read()
    target.write(data)


script, desc_file = argv
desc = open(desc_file, 'r')
data = json.load(desc)
desc.close()

err = True
'''
if "match_ini" in data and data["match_ini"] == "false":
    print "Error:: xrt.ini File Manually Edited:: Auto-file Generator Failed"
    err = False
else:
    print "Generating xrt.ini file for %s" %data["example"]
    target = open("xrt.ini","w+")
    profile_report(target)
'''
if "match_makefile" in data and data["match_makefile"] == "false":
    print "Error:: Makefile Manually Edited:: AutoMakefile Generator Failed"
    err = False
else:
    #print "Generating Auto-Makefile for %s" %data["example"]
    target = open("Makefile", "w")
    create_mk(target, data)
    #print "Generating utils.mk file for %s" %data["example"]
    #target = open("utils.mk", "w+")
    #create_utils(target)

assert err, "Auto-file Generator Failed"
target.close
