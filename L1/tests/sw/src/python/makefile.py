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

import os, sys
import shlex, subprocess
import numpy as np
import pdb

class Makefile:
  def __init__(self, makefile):
    self.makefile = makefile

  def make(self, target, dtype, rtype):

    commandLine =r'make clean -f %s %s'%(self.makefile, target)
    args = shlex.split(commandLine)
    exitCode = subprocess.call(args)
    if exitCode != 0:
      print("ERROR: make clean failure.")
      sys.exit

    os.environ['BLAS_dataType']= "'%s'"%dtype
    os.environ['BLAS_resDataType']="'%s'"%rtype

    commandLine =r'make -f %s %s'%(self.makefile, target)
    args = shlex.split(commandLine)
    exitCode = subprocess.call(args)
    return exitCode
