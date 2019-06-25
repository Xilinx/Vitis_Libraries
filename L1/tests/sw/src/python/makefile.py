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
