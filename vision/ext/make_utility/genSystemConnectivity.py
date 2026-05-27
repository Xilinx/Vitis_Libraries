#!/usr/bin/env python3
import os
import sys
import time
from pathlib import Path

n = len(sys.argv)
if (n == 3):
    AIE_CORES = int(sys.argv[1])
    if(AIE_CORES <= 0):
        print("Error : Unexpected number of AIE_CORES" + AIE_CORES + ". Expected positive integer value")
    else:
        try:
            f = open(sys.argv[2],'w')
            f.write("[connectivity]\n")
            f.write("nk=Tiler_top:")
            f.write(str(AIE_CORES))
            f.write("\n")
            f.write("nk=stitcher_top:")
            f.write(str(AIE_CORES))
            f.write("\n")
            
            for i in range(AIE_CORES):
                f.write("stream_connect=Tiler_top_")
                f.write(str(i+1))
                f.write(".OutputStream:ai_engine_0.S")
                f.write(format(i,'02d'))
                f.write("_AXIS\n")
            
            for i in range(AIE_CORES):
                f.write("stream_connect=ai_engine_0.M")
                f.write(format(i,'02d'))
                f.write("_AXIS:stitcher_top_")
                f.write(str(i+1))
                f.write(".InputStream\n")
            f.close()
        except IOError:
            print("Unable to open file" + sys.argv[2] + " for writing. Please check for permissions.")
else:
    print("Usage: ./genSystemConnectivity.py <Number of AIE_CORES> <Output config file>")
