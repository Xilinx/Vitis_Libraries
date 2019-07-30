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

import numpy as np
import ctypes as C
import argparse
import os, sys
import json
import pdb
import traceback
from blas_gen_bin import BLAS_GEN, BLAS_ERROR
from hls import HLS, HLS_ERROR, Parameters
from makefile import Makefile
from operation import OP, BLAS_L1, BLAS_L2, OP_ERROR

def Format(x):
  f_dic = {0:'th', 1:'st', 2:'nd',3:'rd'}
  k = x % 10
  if k>=4:
    k=0
  return "%d%s"%(x, f_dic[k])

class RunTest:
  def __init__(self, makefile):
    self.profile = None 
    self.parEntries = 1
    self.logParEntries = 0
    self.valueRange = None
    self.numSim = 1
    self.makefile = makefile

    self.hls = None
    self.typeDict ={
      np.int8: 'int8_t',
      np.int16: 'int16_t',
      np.int32: 'int32_t',
      np.int64: 'int64_t',
      np.uint8: 'uint8_t',
      np.uint16: 'uint16_t',
      np.uint32: 'uint32_t',
      np.uint64: 'uint64_t',
      np.float32: 'float',
      np.float64: 'double'
    }

  def parseProfile(self, filePath):
    with open(filePath, 'r') as fh:
      self.profile = json.loads(fh.read())

    self.opName = self.profile['op']
    self.opClass = OP.parse(self.opName)

    self.minValue = self.profile['valueRange'][0]
    self.maxValue = self.profile['valueRange'][1]

    self.op = eval(self.opClass).parse(self.opName, self.maxValue, self.minValue)

    if 'logParEntries' in self.profile:
      self.logParEntries = self.profile['logParEntries']
    else:
      self.logParEntries = -1

    if self.logParEntries == -1:
      self.parEntries = self.profile['parEntries']
    else:
      self.parEntries = 1 << self.logParEntries

    if 'dataTypes' in self.profile:
      self.dataTypes = [eval('np.%s'%dt) for dt in self.profile['dataTypes']]

    if 'retTypes' in self.profile:
      self.retTypes = [eval('np.%s'%dt) for dt in self.profile['retTypes']]
    
    self.numSim = self.profile['numSimulation']

    self.testPath = r'out_test/%s'%self.op.name

    self.libPath = os.path.join(self.testPath, 'libs')
    if not os.path.exists(self.libPath):
      os.makedirs(self.libPath)

    self.dataPath = os.path.join(self.testPath, 'data')
    if not os.path.exists(self.dataPath):
      os.makedirs(self.dataPath)

    self.hls = HLS(r'build/run-hls.tcl', self.profile['b_csim'],
       self.profile['b_synth'], self.profile['b_cosim'])
      #  False, False)

    directivePath = os.path.join(self.testPath, 
        r'directive_par%d.tcl'%(self.parEntries))
    self.params = Parameters(self.op, self.logParEntries, self.parEntries)
    self.hls.generateDirective(self.params, directivePath)

  def build(self):
    c_type = self.typeDict[self.op.dataType]
    self.params.setDtype(c_type)
    r_type = c_type
    self.typeStr = 'd%s'%c_type
    if self.opClass == 'BLAS_L1':
      r_type = self.typeDict[self.op.rtype]
      self.typeStr = 'd%s_r%s'%(c_type, r_type)
    self.params.setRtype(r_type)

    libPath =os.path.join(self.libPath,'blas_gen_%s.so'%self.typeStr)

    if not os.path.exists(libPath):
      make = Makefile(self.makefile, libPath)
      if not make.make(c_type, r_type):
        raise Exception("ERROR: make shared library failure.")

    self.lib = C.cdll.LoadLibrary(libPath)

  def run(self):
    paramTclPath =os.path.join(self.dataPath, r'parameters_%s_%s.tcl'%(self.op.sizeStr,self.typeStr))
    logpath=os.path.join(self.dataPath, r'logs_%s_%s'%(self.op.sizeStr,self.typeStr))
    if not os.path.exists(logpath):
      os.makedirs(logpath)
    logfile=os.path.join(logpath, 'logfile.log')
    binFile =os.path.join(self.dataPath,'TestBin_%s_%s.bin'%(self.op.sizeStr,self.typeStr))

    print("\n")
    print("="*64)
    dataList = [self.op.compute() for j in range(self.numSim)]
    blas_gen=BLAS_GEN(self.lib)
    self.op.addInstr(blas_gen, dataList)
    blas_gen.write2BinFile(binFile)
    print("Data file %s has been generated sucessfully."%binFile)
    del dataList
    self.hls.generateParam(self.params, paramTclPath)
    print("Parameters in file %s.\nLog file %s"%(paramTclPath, logfile))
    self.hls.execution(binFile, logfile)
    self.hls.checkLog(logfile)
    print("Test of size %s passed."%self.op.sizeStr)

  def runTest(self, path):
    self.params.setPath(path)
    self.op.test(self)

def main(profileList, makefile): 
  try:
    for profile in profileList:
      if not os.path.exists(profile):
        raise Exception("ERROR: File %s is not exists."%profile)
      runTest = RunTest(makefile)
      runTest.parseProfile(profile)
      runTest.runTest(os.path.dirname(profile)) 
      print("All tests for %s are passed."%runTest.op.name)
    print("All tests are passed.")
  except OP_ERROR as err:
    print("OPERATOR ERROR: %s"%(err.message))
    print("Test failed due to previous errors.")
  except BLAS_ERROR as err:
    print("BLAS ERROR: %s with status code is %s"%(err.message, err.status))
    print("Test failed due to previous errors.")
  except HLS_ERROR as err:
    print("HLS ERROR: %s\nPlease check log file %s"%(err.message, os.path.abspath(err.logFile)))
  
if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('--makefile', type=str, default='Makefile', metavar='Makefile', help='path to the profile file')
  group = parser.add_mutually_exclusive_group(required=True)
  group.add_argument('--profile', nargs='*', metavar='profile.json', help='list of pathes to the profile files')
  group.add_argument('--operator', nargs='*',metavar='opName', help='list of operator names')
  args = parser.parse_args()
  
  profile = list()
  if args.profile:
    profile = args.profile
  elif args.operator:
    for op in args.operator:
      profile.append('./hw/%s/profile.json'%op)
  else:
    parser.print_help()
  main(set(profile), args.makefile)
