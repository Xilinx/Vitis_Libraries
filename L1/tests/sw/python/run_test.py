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
import shutil
import json
import pdb
import time
import traceback
from blas_gen_bin import BLAS_GEN, BLAS_ERROR
from hls import HLS, HLS_ERROR, Parameters
from makefile import Makefile
from operation import OP, BLAS_L1, BLAS_L2, OP_ERROR
import threading
import concurrent.futures

def Format(x):
  f_dic = {0:'th', 1:'st', 2:'nd',3:'rd'}
  k = x % 10
  if k>=4:
    k=0
  return "%d%s"%(x, f_dic[k])

class RunTest:
  def __init__(self, profile, args):
    self.profilePath = profile
    self.profile = None 
    self.parEntries = 1
    self.logParEntries = 0
    self.valueRange = None
    self.numToSim = 1
    self.numSim = 0
    self.makefile = args.makefile

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

  def parseProfile(self):
    with open(self.profilePath, 'r') as fh:
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
    
    self.numToSim = self.profile['numSimulation']

    self.testPath = os.path.join('out_test', self.op.name)
    self.reports = list()

    self.libPath = os.path.join(self.testPath, 'libs')
    if not os.path.exists(self.libPath):
      os.makedirs(self.libPath)

    self.dataPath = os.path.join(self.testPath, 'data')
    if not os.path.exists(self.dataPath):
      os.makedirs(self.dataPath)
    hlsTCL = os.path.join('.', 'build', r'run-hls.tcl')

    if args.csim:
      self.hls = HLS(hlsTCL,True, False, False) 
    elif args.cosim:
      self.hls = HLS(hlsTCL, False, True, True) 
    else:
      self.hls = HLS(hlsTCL, self.profile['b_csim'],
       self.profile['b_synth'], self.profile['b_cosim'])

    directivePath = os.path.join(self.testPath, 
        r'directive_par%d.tcl'%(self.parEntries))
    self.hls.setParam(Parameters(self.op, self.logParEntries, self.parEntries))
    self.hls.generateDirective(directivePath)

  def build(self):
    envD = dict()
    parEntriesList = ['spmv', 'tpmv']
    c_type = self.typeDict[self.op.dataType]
    self.hls.params.setDtype(c_type)
    envD["BLAS_dataType"] = c_type
    self.typeStr = 'd%s'%c_type
    if self.opClass == 'BLAS_L1':
      r_type = self.typeDict[self.op.rtype]
      self.typeStr = 'd%s_r%s'%(c_type, r_type)
      envD["BLAS_resDataType"] =r_type
      self.hls.params.setRtype(r_type)

    if self.op.name in parEntriesList:
      envD["BLAS_parEntries"] = "%d"%self.parEntries

    with self.makelock:
      make = Makefile(self.makefile, self.libPath)
      libPath = make.make(envD)
    self.lib = C.cdll.LoadLibrary(libPath)

  def runTest(self):
    paramTclPath =os.path.join(self.dataPath, r'parameters_%s_%s.tcl'%(self.op.sizeStr,self.typeStr))
    logfile=os.path.join(self.dataPath, r'logfile_%s_%s.log'%(self.op.sizeStr,self.typeStr))
    binFile =os.path.join(self.dataPath,'TestBin_%s_%s.bin'%(self.op.sizeStr,self.typeStr))

    print("\n")
    print("="*64)
    dataList = [self.op.compute() for j in range(self.numToSim)]
    blas_gen=BLAS_GEN(self.lib)
    self.op.addInstr(blas_gen, dataList)
    blas_gen.write2BinFile(binFile)
    print("\nOP %s: Data file %s has been generated sucessfully."%(self.op.name, binFile))
    del dataList
    self.hls.generateParam(paramTclPath)
    print("\nOP %s: Parameters in file %s."%(self.op.name, paramTclPath))
    print("\nOP %s: Log file %s"%(self.op.name,  logfile))
    self.hls.execution(binFile, logfile, self.testPath)
    self.hls.checkLog(logfile)
    self.numSim += self.numToSim
    self.hls.benchmarking(logfile, self.op, self.reports)
    print("\nOP %s: Test of size %s passed."%(self.op.name, self.op.sizeStr))

  def run(self, makelock):
    self.makelock = makelock
    path = os.path.dirname(self.profilePath)
    self.hls.params.setPath(path)
    self.op.test(self)

  def writeReport(self, profile, flag = 'a+'):
    reportPath = os.path.join(self.testPath, 'report.rpt')
    if len(self.reports) == 0:
      raise OP_ERROR("\nOP %s: Benchmark fails for op %s."%(self.op.name, self.op.name))
    features = self.reports[0]
    keys = features.keys()
    lens = [len(key)+2 for key in keys]

    delimiter = '+' + '+'.join(['-' * l for l in lens]) + '+\n'

    with open(reportPath, flag) as f:
      f.write(time.ctime() + '\n')
      f.write("Profile path: %s\n"%profile)
      f.write(delimiter)
      ########### START OF KEYS ################
      strList=['|']
      for s, l in zip(keys, lens):
        strList.append(('{:<%d}|'%l).format(s))
      strList.append('\n')
      f.write(''.join(strList))
      f.write(delimiter)
      ########### END OF KEYS ################

      while self.reports:
        features = self.reports.pop(0)
        strList=['|']
        for key, l in zip(keys, lens):
          strList.append(('{:<%d}|'%l).format(features[key]))
        strList.append('\n')
        f.write(''.join(strList))
        f.write(delimiter)
    return reportPath

def makeTable(passDict, failDict, skipList, print_fn = print):
  numPasses = len(passDict)
  numFails = len(failDict)
  numOps = numPasses + numFails
  if numOps == 0:
    return
  print_fn('='*40 + '   REPORT   ' + '='*40 + '\n')
  print_fn(time.ctime())
  print_fn('\n')
  if numPasses != 0:
    print_fn("%d / %d operation[s] numPasses the tests.\n" %(numPasses, numOps))
  if numFails != 0:
    print_fn("%d / %d operation[s] failed the tests.\n" %(numFails, numOps))
  delimiter = '+'.join(['', '-'*10, '-'*10, '-'*10, '-'*10, '-'*30, '\n'])
  print_fn(delimiter)
  keys = '|'.join(['', '{:<10}'.format('op Name'), '{:<10}'.format('No.csim'),
      '{:<10}'.format('No.cosim'),'{:<10}'.format('Status'),
      '{:<30}'.format('Profile'), '\n'])
  print_fn(keys)
  print_fn(delimiter)
  for rt in passDict.keys(): 
    csim, cosim = passDict[rt]
    value = '|'.join(['', '{:<10}'.format(rt.op.name), '{:<10}'.format(csim),
      '{:<10}'.format(cosim),'{:<10}'.format('Passed'),'{:<30}'.format(rt.profilePath), '\n'])
    print_fn(value)
    print_fn(delimiter)
  for rt in failDict.keys(): 
    csim, cosim = failDict[rt]
    value = '|'.join(['', '{:<10}'.format(rt.op.name), '{:<10}'.format(csim),
      '{:<10}'.format(cosim),'{:<10}'.format('Failed'), '{:<30}'.format(rt.profilePath),'\n'])
    print_fn(value)
    print_fn(delimiter)
  for skip in skipList:
    value = '|'.join(['', '{:<10}'.format('Unknown'), '{:<10}'.format(0),
      '{:<10}'.format(0),'{:<10}'.format('Skipped'), '{:<30}'.format(skip),'\n'])
    print_fn(value)
    print_fn(delimiter)
  return numFails


def process(runTest, passOps, failOps, dictLock, makeLock):
  passed = False
  try:
    runTest.parseProfile()
    print("Starting to test %s."%(runTest.op.name))
    runTest.run(makeLock) 
    print("All %d tests for %s are passed."%(runTest.numSim, runTest.op.name))
    passed = True
    csim = runTest.numSim * runTest.hls.csim
    cosim = runTest.numSim * runTest.hls.cosim
    with dictLock:
      passOps[runTest] = (csim, cosim)

    if runTest.hls.cosim:
      rpt = runTest.writeReport(profile)
      print("Benchmark info for op %s is written in %s"%(runTest.op.name, rpt))

  except OP_ERROR as err:
    print("OPERATOR ERROR: %s"%(err.message))
  except BLAS_ERROR as err:
    print("BLAS ERROR: %s with status code is %s"%(err.message, err.status))
  except HLS_ERROR as err:
    print("HLS ERROR: %s\nPlease check log file %s"%(err.message, os.path.abspath(err.logFile)))
  except Exception as err:
    type, value, tb = sys.exc_info()
    traceback.print_exception(type, value, tb)
  finally:
    if not passed:
      csim = runTest.numSim * runTest.hls.csim
      cosim = runTest.numSim * runTest.hls.cosim
      with dictLock:
        failOps[runTest] = (csim, cosim)

def main(profileList, args): 
  print(r"There are in total %d testing profile[s]."%len(profileList))
  passOps = dict()
  failOps = dict()
  skipList = list()
  argList = list()
  dictLock = threading.Lock()
  makeLock = threading.Lock()
  while profileList:
    profile = profileList.pop()
    if not os.path.exists(profile):
      print("File %s is not exists."%profile)
      skipList.append(profile)
      continue
    runTest = RunTest(profile, args)
    argList.append(runTest)
  try:
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.parallel) as executor:
      for arg in argList:
        executor.submit(process, arg, passOps, failOps, dictLock, makeLock)
  finally:
    with open("statistics.rpt", 'a+') as f:
      r = makeTable(passOps, failOps, skipList, f.write) 

if __name__== "__main__":
  parser = argparse.ArgumentParser(description='Generate random vectors and run test.')
  parser.add_argument('--makefile', type=str, default='Makefile', metavar='Makefile', help='path to the profile file')
  parser.add_argument('--parallel', type=int, default=1, help='number of parallel processes')
  profileGroup = parser.add_mutually_exclusive_group(required=True)
  profileGroup.add_argument('--profile', nargs='*', metavar='profile.json', help='list of path to profile files')
  profileGroup.add_argument('--operator', nargs='*',metavar='opName', help='list of test dirs in ./hw')
  
  simGroup = parser.add_mutually_exclusive_group()
  simGroup.add_argument('--csim', action='store_true', default=False, help='csim only')
  simGroup.add_argument('--cosim', action='store_true', default=False, help='synthesis and cosim only')
  args = parser.parse_args()
  
  if args.profile:
    profile = args.profile
  else: 
    profile = ['./hw/%s/profile.json'%op for op in args.operator]

  main(set(profile), args)
