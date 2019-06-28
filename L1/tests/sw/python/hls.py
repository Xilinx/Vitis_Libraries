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

import shlex, subprocess
import pdb

class HLS:
  def __init__(self, tclPath, b_csim, b_syn, b_cosim):

    self.tcl = tclPath
    self.csim = b_csim
    self.syn = b_syn 
    self.cosim = b_cosim 
    self.opArgs=str()

  def execution(self, runArgs, logFile):
    commandLine ='''vivado_hls -f %s "runCsim %d runRTLsynth %d \
runRTLsim %d part vu9p %s runArgs '%s'"'''%(self.tcl, self.csim, self.syn,
  self.cosim, self.opArgs, runArgs)
    #pdb.set_trace()
    #print(commandLine)
    args = shlex.split(commandLine)
    hls = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdoutdata, stderrdata) = hls.communicate()
    print(stderrdata)
    with open(logFile, 'w') as f:
      f.write(stdoutdata)
  def checkLog(self, logFile):
    with open(logFile, 'r') as f:
      content = f.read()
    errIndex = content.find("ERROR")
    failIndex = content.find("FAIL")
    if errIndex == -1 and failIndex == -1: 
      return True
    else:
      return False

  def generateTCL(self, op, c_type, dw, r_type, logParEntries):
    self.opArgs=r'op %s dataType %s dataWidth %d resDataType %s logParEntries %d'%(op, c_type, dw, r_type, logParEntries)

