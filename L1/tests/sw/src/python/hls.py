/*
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

*/
import shlex, subprocess

class HLS:
  def __init__(self, tclPath, b_csim, b_syn, b_cosim):

    self.tcl = tclPath
    self.csim = b_csim
    self.syn = b_syn 
    self.cosim = b_cosim 

  def execution(self, opArgs, runArgs):
    commandLine ='''vivado_hls -f %s "runCsim %d runRTLsynth %d \
runRTLsim %d part vu9p %s runArgs '%s'"'''%(self.tcl, self.csim, self.syn,
  self.cosim, opArgs, runArgs)
    args = shlex.split(commandLine)
    exitCode = subprocess.call(args)
    return exitCode

