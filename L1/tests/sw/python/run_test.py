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
from blas_gen_bin import BLAS_GEN
from hls import HLS
from makefile import Makefile
import pdb
from operation import BLAS_L1

def Format(x):
  f_dic = {0:'th', 1:'st', 2:'nd',3:'rd'}
  k = x % 10
  if k>=4:
    k=0
  return "%d%s"%(x, f_dic[k])
class RunTest:
  def __init__(self):
    self.profile = None 
    self.opList = None 
    self.dtList =  None
    self.vectorSize = None  
    self.dtWidth = None  
    self.parEntries = 1
    self.logParEntries = 0
    self.valueRange = None
    self.numSim = 1

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
    self.op = self.profile['op']
    self.dtList = self.profile['dataTypes']
    self.rtList = self.profile['retTypes']


    if 'logParEntries' in self.profile:
      self.logParEntries = self.profile['logParEntries']
      self.parEntries = 1 << self.logParEntries
    if 'parEntries' in self.profile:
      self.parEntries == self.profile['parEntries']
    
    self.minValue = self.profile['valueRange'][0]
    self.maxValue = self.profile['valueRange'][1]

    self.numSim = self.profile['numSimulation']

    self.vectorSizes = self.profile['vectorSizes']

    self.testPath = r'out_test/%s'%self.op

    self.libPath = os.path.join(self.testPath, 'libs')
    if not os.path.exists(self.libPath):
      os.makedirs(self.libPath)
    self.dataPath = os.path.join(self.testPath, 'data')
    if not os.path.exists(self.dataPath):
      os.makedirs(self.dataPath)

    self.hls = HLS(r'build/run-hls.tcl', self.profile['b_csim'],
       self.profile['b_synth'], self.profile['b_cosim'])
     #   False, False)

    directivePath = os.path.join(self.testPath, 
        r'directive_par%d.tcl'%(self.logParEntries))
    self.hls.generateDirective(self.parEntries, directivePath)


  def runTest(self,makefile, maxN):
    dtLen =  len(self.dtList)
    numSimulation = 0
    for index in range(dtLen):
      dt = self.dtList[index][0]
      dw = self.dtList[index][1]
      dtype = eval(r'np.%s%d'%(dt, dw))
      rt = self.rtList[index][0]
      rw = self.rtList[index][1]
      rtype = eval(r'np.%s%d'%(rt, rw))

      c_type=self.typeDict[dtype]
      r_type=self.typeDict[rtype]

      typeStr = 'd%s%s_r%s%d'%(dt,dw,rt,rw)

      libPath =os.path.join(self.libPath,'blas_gen_%s.so'%typeStr)

      if not os.path.exists(libPath):
        make = Makefile(makefile, libPath)
        if not make.make(c_type, r_type):
          print("ERROR: make shared library failure.")
          sys.exit
      lib = C.cdll.LoadLibrary(libPath)
      print("Start to test operation %s under DataType %s and return DataType %s"%(self.op, c_type, r_type))
      for vectorSize in self.vectorSizes:
        if numSimulation == maxN:
          break
        numSimulation = numSimulation + 1
        paramTclPath =os.path.join(self.testPath, 
           r'parameters_v%d_%s.tcl'%(vectorSize,typeStr))
        self.hls.generateParam(self.op, c_type, dw, r_type, self.logParEntries, 
            self.parEntries, vectorSize, paramTclPath)

        logfile=os.path.join(self.dataPath, 
            r'logfile_v%d_%s.log'%(vectorSize,typeStr))
        
        binFile =os.path.join(self.dataPath,'TestBin_v%d_%s.bin'%(vectorSize,typeStr))
        blas_gen=BLAS_GEN(lib)
        op = BLAS_L1.parse(self.op,dtype, vectorSize, self.maxValue, self.minValue) 

        dataList = list()
        for j in range(self.numSim): 
          dataList.append(op.compute())
          alpha, xdata, ydata, xr, yr,r = dataList[-1]
          res = blas_gen.addB1Instr(self.op, vectorSize, alpha, xdata, ydata, xr, yr, r.astype(rtype))
          if not res == 'XFBLAS_STATUS_SUCCESS':
            print("ERROR: Add instruction failed.")
            print("Test failed due to the previous errors.")
            return

        res = blas_gen.write2BinFile(binFile)
        if not res == 'XFBLAS_STATUS_SUCCESS':
          print("ERROR: Write data file %s failed."%binFile)
          print("Test failed due to the previous errors.")
          return
        print("Data file %s has been generated sucessfully."%binFile)
        print("Test vector size %d. Parameters in file %s.\nLog file %s\n"%(vectorSize, paramTclPath, logfile))
        self.hls.execution(binFile, logfile, True)
        result = self.hls.checkLog(logfile)
        if result:
          print("Test passed.")
        else:
          print("Test failed.\n %s with input %s\nplease check log file %s"%(self.op, 
                binFile, os.path.abspath(logfile)))
          return
    print("All tests are passed.")

def main(profileList, makefile, maxN): 
  for profile in profileList:
    if not os.path.exists(profile):
      print("File %s is not exists."%profile)
      continue
    runTest = RunTest()
    runTest.parseProfile(profile)
    runTest.runTest(makefile, maxN)
  
if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('--makefile', type=str, default='Makefile', metavar='Makefile', help='path to the profile file')
  parser.add_argument('--max', metavar='numSim', type=int, help='maximum number of simulations for each profile/operator')
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
  main(set(profile), args.makefile, args.max)
