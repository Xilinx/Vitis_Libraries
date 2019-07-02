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
import os

class HLS:
  def __init__(self, tclPath, b_csim, b_syn, b_cosim):

    self.tcl = tclPath
    self.csim = b_csim
    self.syn = b_syn 
    self.cosim = b_cosim 

  def execution(self, binFile, logFile, b_print = False):
    commandLine ='vivado_hls -f %s %s %s %s'%(self.tcl, self.params, 
        self.directive, os.path.abspath(binFile))
 #   print(commandLine)
    #pdb.set_trace()
    args = shlex.split(commandLine)
    hls = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
#    (stdoutdata, stderrdata) = hls.communicate()
    with open(logFile, 'w') as f:
      while True:
        line = hls.stdout.readline()
        if not line:
          break
        if b_print:
          print(line, end='')
        f.write(line)
  def checkLog(self, logFile):
    with open(logFile, 'r') as f:
      content = f.read()
    if self.cosim:
      passIndex = content.find(r"C/RTL co-simulation finished: PASS")
      if passIndex >=0:
        return True
      else:
        return False
    elif self.syn:
      passIndex = content.find("Finished generating all TRL models")
      if passIndex >=0:
        return True
      else:
        return False
    elif self.csim:
      passIndex = content.find("CSIM done with 0 errors")
      if passIndex >=0:
        return True
      else:
        return False
    else:
      return False

  def generateTCL(self, op, c_type, dw, r_type, logParEntries, vs, fileparams, directive):
    self.params = fileparams
    self.directive = directive
    with open(self.params, 'w') as f:
       f.write('array set opt {\n ')   
       f.write('   part    vu9p\n ')
       f.write('   dataType %s\n '%c_type)
       f.write('   resDataType %s\n '%r_type)
       f.write('   dataWidth %d\n '%dw)
       f.write('   logParEntries %d\n '%logParEntries)
       f.write('   vectorSize %d\n '%vs)
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
       f.write(' }\n ')
    with open(self.directive, 'w') as f:
       #f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_x\n'%(vs))
       #f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_y\n'%(vs))
       #f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_xRes\n'%(vs))
       #f.write('set_directive_interface -mode m_axi -depth %d "uut_top" p_yRes\n'%(vs))
       f.write('set_directive_array_partition -type cyclic -factor %d -dim 1 "uut_top" p_x\n'%(1<<logParEntries))
       f.write('set_directive_array_partition -type cyclic -factor %d -dim 1 "uut_top" p_y\n'%(1<<logParEntries))
       f.write('set_directive_array_partition -type cyclic -factor %d -dim 1 "uut_top" p_xRes\n'%(1<<logParEntries))
       f.write('set_directive_array_partition -type cyclic -factor %d -dim 1 "uut_top" p_yRes\n'%(1<<logParEntries))

