#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
set aiesimLogFile "${fileDir}AIESimulator.log"
set x86simLogFile "${fileDir}x86simulator_output/x86simulator.log"
set diffFile "${fileDir}logs/diff.txt"
# Result phrases to grep
set compPhrase "compilation complete"
set aiesimPhrase "simulation finished"
set x86simPhrase "ERROR SUMMARY: "
set funcPhrase "identical"
# Warning phrases to grep
set deadlockPhrase "Detected deadlock"
# if there are valgrind warnings there will be ERROR SUMMARY: <non-zero number>
set valgrindPhrase "ERROR SUMMARY: \[1-9\]"

# Default status
set compilation "default"
set simulation "default"
set functional "default"
set valgrind "default"
# warning counter. These will only be printed if warnings > 0
set warnings 0
set valgrind 0
set deadlock 0

# If compile file exists, search for compile phrase and remove defaults
if {[file exist $aieCompileLogFile]} {
    set grepOut [catch {exec grep -i $compPhrase -c $aieCompileLogFile}]
    set compilation [expr !$grepOut]
    set functional 0
    set simulation 0

}
# check x86sim file for completed simulation
if {[file exist $x86simLogFile]} {
    set grepOut [catch {exec grep -i $x86simPhrase -c $x86simLogFile}]
    set simulation [expr !$grepOut]
}
# check aiesim file for completed simulation
if {[file exist $aiesimLogFile]} {
    set grepOut [catch {exec grep -i $aiesimPhrase -c $aiesimLogFile}]
    set simulation [expr !$grepOut]
}
# If diff file exists, search for diff phrase
if {[file exist $diffFile]} {
    set grepOut [catch {exec grep -i $funcPhrase -c $diffFile}]
    set functional [expr !$grepOut]
    # force sim and comp to be 1 if functional
    if {$functional} {
        set simulation 1
        set compilation 1
    }
}
# detect simulation warnings
# valgrind - present when x86sim file produced ERROR SUMMARY: <not 0>
if {[file exist $x86simLogFile]} {
    # search for valgrind warnings
    set grepOut [catch { exec grep -E -i "$valgrindPhrase" $x86simLogFile  }]
    set valgrind [expr !$grepOut]
    # search for deadlocks
    set grepOut [catch { exec grep -E -i "$deadlockPhrase" $x86simLogFile  }]
    set deadlock [expr !$grepOut]
}
if {[file exist $aiesimLogFile]} {
    # search for deadlocks (AIESIM)
    set grepOut [catch { exec grep -E -i "$deadlockPhrase" $aiesimLogFile  }]
    set deadlock [expr !$grepOut]
}
set warnings [expr $valgrind + $deadlock]
# -------------------------
# --- Print to Terminal ---
# -------------------------
puts "Configuration:"
for {set i 2} { $i < [llength $argv] } { incr i 2 } {
    puts "    [lindex $argv $i]:        [lindex $argv [expr ($i+1)]]"
}
if {$warnings > 0} {
    puts "Warnings:"
    puts "    VALGRIND:             $valgrind"
    puts "    DEADLOCK:             $deadlock"
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
if {$warnings > 0} {
    puts $outFile "Warnings:"
    puts $outFile "    VALGRIND:             $valgrind"
    puts $outFile "    DEADLOCK:             $deadlock"
}
puts $outFile "Results:"
puts $outFile "    COMPILE:              $compilation"
puts $outFile "    SIM:                  $simulation"
puts $outFile "    FUNC:                 $functional"
close $outFile