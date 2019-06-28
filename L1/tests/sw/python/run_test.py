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
    self.parEntries = None
    self.valueRange = None
    self.numSim = None

    self.hls = None
    self.typeDict ={
      np.int8: 'char',
      np.int16: 'short',
      np.int32: 'int',
      np.int64: 'long long int',
      np.uint8: 'unsigned char',
      np.uint16: 'unsigned short',
      np.uint32: 'unsigned int',
      np.uint64: 'unsigned long long int',
      np.float32: 'float',
      np.float64: 'double'
    }

  def parseProfile(self, filePath):
    with open(filePath, 'r') as fh:
      self.profile = json.loads(fh.read())
    self.op = self.profile['op']
    self.dtList = self.profile['dataTypes']
    self.rtList = self.profile['retTypes']


    self.logParEntries = self.profile['logParEntries']
    
    self.minValue = self.profile['valueRange'][0]
    self.maxValue = self.profile['valueRange'][1]

    self.numSim = self.profile['numSimulation']

    self.minSize = self.profile['vectorSizes'][0]
    self.maxSize = self.profile['vectorSizes'][1]

    self.hls = HLS(r'build/run-hls.tcl', self.profile['b_csim'],
        self.profile['b_synth'], self.profile['b_cosim'])
    self.libpath = r'out_test/libs'
    if not os.path.exists(self.libpath):
      os.makedirs(self.libpath)
    self.datapath = r'out_test/data' 
    if not os.path.exists(self.datapath):
      os.makedirs(self.datapath)
    self.vs=list()

  def runTest(self,makefile):
    dtLen =  len(self.dtList)
    for index in range(dtLen):
      dt = self.dtList[index][0]
      dw = self.dtList[index][1]
      dtype = eval(r'np.%s%d'%(dt, dw))
      rt = self.rtList[index][0]
      rw = self.rtList[index][1]
      rtype = eval(r'np.%s%d'%(rt, rw))

      c_type=self.typeDict[dtype]
      r_type=self.typeDict[rtype]
      libpath =os.path.join(self.libpath,
          'blas_gen_d%s%s_r%s%d.so'%(dt,dw,rt,rw))
      if not os.path.exists(libpath):
        make = Makefile(makefile, libpath)
        if not make.make(c_type, r_type):
          print("ERROR: make shared library failure.")
          sys.exit
      lib = C.cdll.LoadLibrary(libpath)
      print("Start to test operation %s under DataType %s and return DataType %s"%(self.op, c_type, r_type))
      for j in range(self.numSim): 
        while(True):
          vectorSize = (np.random.randint(self.minSize, self.maxSize)>> self.logParEntries)<< self.logParEntries
          if not vectorSize in self.vs:
            self.vs.append(vectorSize)
            break

        op = BLAS_L1.parse(self.op,dtype, vectorSize, self.maxValue, self.minValue) 
        alpha, xdata, ydata, xr, yr, r = op.compute()
        binFile =os.path.join(self.datapath,
          'TestBin_v%d_d%s%s_r%s%d.bin'%(vectorSize,dt,dw,rt,rw))
        blas_gen=BLAS_GEN(lib)
        blas_gen.addB1Instr(self.op, vectorSize, alpha, xdata, ydata, xr, yr,
            r.astype(rtype))
        blas_gen.write2BinFile(binFile)
        print("Data file %s has been generated sucessfully."%binFile)
        #blas_read=BLAS_GEN(lib)
        #blas_read.readFromBinFile(binFile)
        #blas_read.printProgram()
        #blas_gen.printProgram()
        
        runArgs=os.path.abspath(binFile)
        logfile=os.path.join(self.datapath, r'logfile_v%d_d%s%s_r%s%d.log'%(vectorSize,dt,dw,rt,rw))

        print("Starting %s test."%Format(j+1))
        self.hls.generateTCL(self.op, c_type, dw, r_type, self.logParEntries)
        self.hls.execution(runArgs, logfile)
        result = self.hls.checkLog(logfile)

        if result:
          print("%s test passed."%Format(j+1))
        else:
          print("Operation %s failed the test with input %s, please check log file %s"%(self.op, 
                binFile, os.path.abspath(logfile)))
          return
    print("All tests are passed.")

def main(profile, makefile):
  runTest = RunTest()
  runTest.parseProfile(profile)
  runTest.runTest(makefile)
  
if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('p', type=str, metavar='Profile', help='path to the profile file')
  args = parser.parse_args()
  makefile=r'./Makefile'
  main(args.p, makefile)
