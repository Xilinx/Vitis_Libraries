#!/usr/bin/env /tools/cpkg/.packages/x86_64/RHEL7.2/python/3.7.1/bin/python3.7 
from sys import argv
import json
from collections import OrderedDict
import os
import subprocess

XSA = 'xilinx_u200_xdma_201830_2'
VERSION = 'VITIS 2021.1'

def overview(target,data):
    target.write(data["name"])
    target.write("\n")
    target.write("=" * len(data["name"]))
    target.write("\n\n")
    target.write("**Description:** ")
    target.write(data["description"])
    target.write("\n\n")
    target.write("**Top Function:** ")
    target.write(data["topfunction"])
    target.write("\n\n")
    return

def benchmark(target,data):
    if 'QoR_golden' in data:
        target.write("Results\n")
        target.write("-" * len("Results"))
        target.write("\n\n")
        target.write("=" * 24 + ' ')
        target.write("=" * 9 + ' ')
        target.write("=" * 9 + ' ')
        target.write("=" * 5 + ' ')
        target.write("=" * 5 + ' ')
        target.write("\n")
        target.write("Module" + ' ')
        target.write(' '*(24-6) + "LUT" + ' ')
        target.write(' '*(9-3) + "FF" + ' ')
        target.write(' '*(9-2) + "BRAM" + ' ')
        target.write(' '*(5-4) + "URAM" + ' ')
        target.write("\n")
        target.write(data["project"] + ' ')
        target.write(' '*(24-len(data["project"])) + str(round(float(data["QoR_golden"]["VV_LUT"].replace(',',""))/1000,1)) + 'K ')
        target.write(' '*(9-len(str(round(float(data["QoR_golden"]["VV_LUT"].replace(',',""))/1000,1)) + 'K')) + str(round(float(data["QoR_golden"]["VV_FF"].replace(',',""))/1000,1)) + 'K ')
        target.write(' '*(9-len(str(round(float(data["QoR_golden"]["VV_FF"].replace(',',""))/1000,1)) + 'K')) + data["QoR_golden"]["VV_BRAM"] + ' ')
        target.write(' '*(5-len(str(data["QoR_golden"]["VV_BRAM"]))) + data["QoR_golden"]["VV_URAM"] + ' ')
        target.write("\n")
        target.write("=" * 24 + ' ')
        target.write("=" * 9 + ' ')
        target.write("=" * 9 + ' ')
        target.write("=" * 5 + ' ')
        target.write("=" * 5 + ' ')
    return

# Get the argument from the description
script, desc_file = argv

# load the description file
print ("VITIS README File Genarator")
desc = open(desc_file,'r')

# load the json data from the file
data = json.load(desc)
desc.close()

print ("Generating the README for %s" % data["name"])
target = open("README.rst","w")
overview(target,data)
benchmark(target,data)

target.close
