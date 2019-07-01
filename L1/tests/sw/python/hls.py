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

class HLS:
  def __init__(self, tclPath, b_csim, b_syn, b_cosim):

    self.tcl = tclPath
    self.csim = b_csim
    self.syn = b_syn 
    self.cosim = b_cosim 

  def execution(self, logFile):
    commandLine ='vivado_hls -f %s "%s" "%s"'%(self.tcl, self.params, self.directive)
    #print(commandLine)
    #pdb.set_trace()
    args = shlex.split(commandLine)
    hls = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
#    (stdoutdata, stderrdata) = hls.communicate()
    with open(logFile, 'w') as f:
      while True:
        line = hls.stdout.readline()
        if not line:
          break
        print(line, end='')
        f.write(line)
  def checkLog(self, logFile):
    with open(logFile, 'r') as f:
      content = f.read()
    errIndex = content.lower().find("error")
    failIndex = content.lower().find("fail")
    if errIndex == -1 and failIndex == -1: 
      return True
    else:
      return False

  def generateTCL(self, op, c_type, dw, r_type, logParEntries, vs, binFile, fileparams, directive):
    self.params = fileparams
    self.directive = directive
    with open(self.params, 'w') as f:
       f.write('array set opt {\n ')   
       f.write('   part    vu9p\n ')
       f.write('   dataType %s\n '%c_type)
       f.write('   resDataType %s\n '%r_type)
       f.write('   dataWidth %d\n '%dw)
       f.write('   logParEntries %d\n '%logParEntries)
       f.write('   pageSizeBytes 4096\n ')
       f.write('   memWidthBytes 64\n ')
       f.write('   instrSizeBytes 8\n ')
       f.write('   maxNumInstrs 16\n ')
       f.write('   instrPageIdx 0\n ')
       f.write('   paramPageIdx 1\n ')
       f.write('   statsPageIdx 2\n ')
       f.write('   opName "%s"\n '%op)
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
       f.write('   runArgs "%s"\n '%binFile)
       f.write(' }\n ')
    with open(self.directive, 'w') as f:
       f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_x\n'%(vs>>logParEntries))
       f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_y\n'%(vs>>logParEntries))
       f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_xRes\n'%(vs>>logParEntries))
       f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_yRes\n'%(vs>>logParEntries))

