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
  def __init__(self, makefile, libpath):
    self.makefile = makefile
    self.libpath = libpath

  def make(self, dtype='int', rtype='int', rebuild = False):

    self.target = r'out_test/blas_gen_bin_d%s_r%s.so'%(dtype, rtype)
    if os.path.exists(self.target) and rebuild:
      os.remove(self.target)

    commandLine =r'make -f %s %s BLAS_dataType=%s BLAS_resDataType=%s'%(self.makefile, self.target, dtype, rtype)
    args = shlex.split(commandLine)
    subprocess.call(args)
    if os.path.exists(self.target):
      os.rename(self.target, self.libpath)
      return True
    else:
      return False
