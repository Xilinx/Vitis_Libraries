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

from __future__ import print_function
import shlex, subprocess
import pdb
import os, sys

class HLS_ERROR(Exception):
  def __init__(self, message, logFile):
    self.message = message
    self.logFile = logFile

class Parameters:
  def __init__(self, op, logParEntries, parEntries):
    self.op = op
    self.logParEntries = logParEntries
    self.parEntries=parEntries
  def setRtype(self, rtype):
    self.rtype = rtype
  def setDtype(self, dtype):
    self.dtype = dtype
  def setPath(self, path):
    self.path = path

class HLS:
  def __init__(self, tclPath, b_csim, b_syn, b_cosim):

    self.tcl = tclPath
    self.csim = b_csim
    self.syn = b_syn 
    self.cosim = b_cosim 

  def execution(self, binFile, logFile, b_print = False):
    commandLine ='vivado_hls -f %s %s %s %s'%(self.tcl, self.params, 
        self.directive, os.path.abspath(binFile))
    print(commandLine)
    if not b_print:
      print("vivado_hls stdout print is hidden.")
    #pdb.set_trace()
    args = shlex.split(commandLine)
    hls = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
#    (stdoutdata, stderrdata) = hls.communicate()
    with open(logFile, 'w', buffering=1) as f:
      f.write(commandLine)
      while True:
        line = hls.stdout.readline()
        if not line:
          break
        line = line.decode('utf-8')
        if b_print:
          print(line, end='')
        else:
          print('.', end='') 
          if line.find("CSIM finish") >=0:
            print('\nCSIM finished.')
          if line.find(r'C/RTL co-simulation finished') >=0:
            print('\nCOSIM finished.')
          if line.find(r'C/RTL SIMULATION') >=0:
            print("\nSYNTHESIS finished.")
          sys.stdout.flush()
        f.write(line)
    print('vivado_hls finished execution.') 
  def checkLog(self, logFile):
    with open(logFile, 'r') as f:
      content = f.read()
    passIndex = content.find("ERROR")
    if passIndex >= 0:
      raise HLS_ERROR("HLS execution met errors.", logFile)
    if self.cosim:
      passIndex = content.find(r"C/RTL co-simulation finished: PASS")
      if passIndex < 0:
        raise HLS_ERROR("C/RTL co-simulation FAILED.", logFile)
    elif self.syn:
      passIndex = content.find("Finished generating all RTL models")
      if passIndex < 0:
        raise HLS_ERROR("SYNTHESIS FAILED.", logFile)
    elif self.csim:
      passIndex = content.find("CSim done with 0 errors")
      if passIndex < 0:
        raise HLS_ERROR("Csim FAILED.", logFile)
    else:
      raise HLS_ERROR("PROFILE ERROR.", logFile)

  def cosimPerf(self, logFile):
    if self.cosim:
      pass

  def generateParam(self, m, fileparams):
    self.params = fileparams
    with open(self.params, 'w') as f:
       f.write('array set opt {\n ')   
       f.write('   path %s\n '%m.path)
      ###########  TEST PARAMETERS  ##############
       f.write('   dataType %s\n '%m.dtype)
       f.write('   resDataType %s\n '%m.rtype)
       f.write('   logParEntries %d\n '%m.logParEntries)
       f.write('   parEntries %d\n '%m.parEntries)
      ###########  OP PARAMETERS  ##############

       m.op.paramTCL(f)

      ###########  HLS PARAMETERS  ##############
       if self.csim:
         f.write('   runCsim     1\n ')
       else:
         f.write('   runCsim     0\n ')
       if self.syn:
        f.write('   runRTLsynth   1\n ')
       else:
        f.write('   runRTLsynth   0\n ')
       if self.cosim:
         f.write('   runRTLsim     1\n ')
       else:
         f.write('   runRTLsim     0\n ')
      ###########  FIXED PARAMETERS  ##############
       f.write('   part    vu9p\n ')
       f.write('   pageSizeBytes 4096\n ')
       f.write('   memWidthBytes 64\n ')
       f.write('   instrSizeBytes 8\n ')
       f.write('   maxNumInstrs 16\n ')
       f.write('   instrPageIdx 0\n ')
       f.write('   paramPageIdx 1\n ')
       f.write('   statsPageIdx 2\n ')
       f.write(' }\n ')

  def generateDirective(self, m:Parameters, directivePath):
    self.directive = directivePath
    with open(self.directive, 'w') as f:
      for inface in m.op.interfaceList:
       #f.write('set_directive_interface -mode m_axi -depth %d "uut_top" %s\n'%(vs, inface))
       f.write('set_directive_array_partition -type cyclic -factor %d -dim 1 "uut_top" %s\n'%(m.parEntries, inface))
