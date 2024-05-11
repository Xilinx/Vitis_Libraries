#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#
set outStatus           [lindex $argv 0]
set fileDir             [lindex $argv 1]

set logDir [file dirname $outStatus]
# Check if the directory exists  
if { ![file exists $logDir] } {  
    # If the directory doesn't exist, create it  
    exec mkdir -p -- $logDir  
} 

# ---------------------------
# --- Grep Status Results ---
# ---------------------------
set aieCompileLogFile "${fileDir}AIECompiler.log"
set aieSimLogFile "${fileDir}AIESimulator.log"
set diffFile "${fileDir}logs/diff.txt"
set compPhrase "compilation complete"
set simPhrase "simulation finished"
set funcPhrase "identical"
if {[catch {exec grep -i $funcPhrase -c $diffFile}]} {
    # default simulation and compilation to fail if functional fail
    # we grep later to find out if they did pass the specific stage or not
    set functional 0
    set simulation 0
    set compilation 0
} else {
    set functional 1
    set simulation 1
    set compilation 1
}
if {[catch {exec grep -i $simPhrase -c $aieSimLogFile}]} {
    #nothing to do - if functional pass assume simulation passed otherwise will default to fail.
    set simulation $simulation
} else {
    # simulation completed fine
    set simulation 1
}
if {[catch {exec grep -i $compPhrase -c $aieCompileLogFile}]} {
    #nothing to do - if functional pass assume compilation passed otherwise will default to fail.
    set compilation $compilation
} else {
    # compliation completed fine
    set compilation 1
}


# -------------------------
# --- Print to Terminal ---
# -------------------------
puts "Configuration:"
for {set i 2} { $i < [llength $argv] } { incr i 2 } {
    puts "    [lindex $argv $i]:        [lindex $argv [expr ($i+1)]]"
}
puts "Results:"
puts "    COMPILE:              $compilation"
puts "    SIM:                  $simulation"
puts "    FUNC:                 $functional"


# ----------------------------
# --- Write to Output File ---
# ----------------------------
set outFile [open $outStatus w]
puts $outFile "Configuration:"
for {set i 2} {$i < [llength $argv]} {incr i 2} {
    puts $outFile "    [lindex $argv $i]:        [lindex $argv [expr ($i+1)]]"
}
puts $outFile "Results:"
puts $outFile "    COMPILE:              $compilation"
puts $outFile "    SIM:                  $simulation"
puts $outFile "    FUNC:                 $functional"
close $outFile