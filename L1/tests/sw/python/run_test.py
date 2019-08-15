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
from blas_gen_bin import BLAS_ERROR
from hls import HLS_ERROR
import threading
import concurrent.futures
from runTest import RunTest

def Format(x):
  f_dic = {0:'th', 1:'st', 2:'nd',3:'rd'}
  k = x % 10
  if k>=4:
    k=0
  return "%d%s"%(x, f_dic[k])

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


def process(rt, passOps, failOps, dictLock, makeLock):
  passed = False
  try:
    rt.parseProfile()
    with rt.opLock:
      print("Starting to test %s."%(rt.op.name))
      rt.run(makeLock) 
      print("All %d tests for %s are passed."%(rt.numSim, rt.op.name))
      passed = True
      csim = rt.numSim * rt.hls.csim
      cosim = rt.numSim * rt.hls.cosim
      with dictLock:
        passOps[rt] = (csim, cosim)

      if rt.hls.cosim:
        rpt = rt.writeReport(profile)
        print("Benchmark info for op %s is written in %s"%(rt.op.name, rpt))

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
      csim = rt.numSim * rt.hls.csim
      cosim = rt.numSim * rt.hls.cosim
      with dictLock:
        failOps[rt] = (csim, cosim)

def main(profileList, args): 
  print(r"There are in total %d testing profile[s]."%len(profileList))
  opLocks = dict()
  passOps = dict()
  failOps = dict()
  skipList = list()
  argList = list()
  dictLock = threading.Lock()
  makeLock = threading.Lock()
  for profile in profileList:
    if not os.path.exists(profile):
      print("File %s is not exists."%profile)
      skipList.append(profile)
      continue
    rt = RunTest(profile, args)
    argList.append(rt)
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
