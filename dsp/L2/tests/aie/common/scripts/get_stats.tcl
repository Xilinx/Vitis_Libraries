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

# Get min
proc min {list} {
    set min [lindex $list 0]
    foreach i $list {
        if { $i < $min } {
            set min $i
        }
    }
    return $min
}

# Get max
proc max {list} {
    set max 0
    foreach i $list {
        if { $i > $max } {
            set max $i
        }
    }
    return $max
}

# Get args
set windowSize          [lindex $argv 0]
set cascLen             [lindex $argv 1]
set outStatus           [lindex $argv 2]
set fileDir             [lindex $argv 3]
if { [llength $argv] > 4 } {
    set funcName           [lindex $argv 4]
} else {
    set funcName "filter"
}
if { [llength $argv] > 5 } {
    set numIter            [lindex $argv 5]
} else {
    set numIter           1
}

# -----------------------------------
# --- Get Aiesimulator File Names ---
# -----------------------------------
set filterFunctionName $funcName
set fileNames [glob -directory $fileDir -- "profile_funct_*.xml"]
set fileSimSummary [glob -directory $fileDir -- "*_summary"]
# Strings to helps extract cycle counts
set strLvl1  "    <function>"
set strLvl2  "        <function_name>$filterFunctionName"
set strLvl2main  "        <function_name>main"
set strLvl3  "        <function_and_descendants_time>"
set x 0
set lineNo 0
set kernelStatsDone 0

# ------------------------------
# --- Initialize Cycle Count ---
# ------------------------------
set cycleCountTotal {}
set cycleCountMin {}
set cycleCountMax {}
set cycleCountAvg {}
# AIE Frequency in MHz
set aieFrequency 1000

# -----------------------
# --- Parse XML Files ---
# -----------------------
foreach fileName  $fileNames {
    if {$kernelStatsDone == 0} {
    # skip reading file if already found
    set inFile [open $fileName r]
        while {[gets $inFile line] != -1} {
            incr lineNo
            set strLine [string range $line 0 [string length $strLvl2main]-1]
            if {![string compare $strLine $strLvl2main] } {
                while {[gets $inFile line] != -1} {
                    incr lineNo
                    set strLine [string range $line 0 42]
                    if {![string compare $strLine $strLvl3]} {
                        gets $inFile line
                        set mainCycleCountTotal    [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                        # Average out over number of iterations:
                        lappend initiationIntervalAprx    [expr {$mainCycleCountTotal  / $numIter}]
                        break
                    }
                }
            }
            set strLine [string range $line 0 [string length $strLvl2]-1]
            if {![string compare $strLine $strLvl2] } {
                while {[gets $inFile line] != -1} {
                    incr lineNo
                    set strLine [string range $line 0 42]
                    if {![string compare $strLine $strLvl3]} {
                        gets $inFile line
                        lappend cycleCountTotal    [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                        gets $inFile line
                        lappend cycleCountMin  [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                        gets $inFile line
                        lappend cycleCountMax  [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                        gets $inFile line
                        lappend cycleCountAvg  [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                        break
                    }
                }
                incr x
                break
            }
        }
    }
}
close $inFile

# --------------------------------
# --- Parse Simulation Summary ---
# --- to get AIE Frequency     ---
# --------------------------------
foreach simSummary  $fileSimSummary {
    set lineNo 0
    set inFile [open $simSummary r]
    while {[gets $inFile line] != -1} {
        incr lineNo
        if {[string first "frequency" $line ] != -1 } {
            # extract from simulation summary
            set aieFrequency [lindex [split [lindex [split $line ":"] 1] ","] 0]
            break
        }
    }
}
close $inFile

# ----------------------------------------
# --- Set Default Cycle Count if Empty ---
# ----------------------------------------
if {[llength $cycleCountAvg] == 0} {
    for {set i 0} {$i < $cascLen} {incr i} {
        # in case profile not found
        lappend cycleCountTotal -1
        lappend cycleCountMin -1
        lappend cycleCountMax -1
        lappend cycleCountAvg -1
    }
}

# ------------------------------------------
# --- Compute Throughput for each Kernel ---
# ------------------------------------------
for {set x 0} {$x < $cascLen} {incr x} {
    # Actual average throughput in MSa per second
    lappend throughputCycleCount  [expr {$aieFrequency * $windowSize / [lindex $cycleCountAvg $x]}]
    lappend throughputIIAvg  [expr {$aieFrequency * $windowSize / [lindex $initiationIntervalAprx $x]}]
}

# -----------------------------------------
# --- Compute Throughput for the design ---
# -----------------------------------------
# Get the design extrema cycle count.
set maxDescycleCountTotal [max $cycleCountTotal]
set minDesCycleCountMin [min $cycleCountMin]
set maxDescycleCountMax [max $cycleCountMax]
set maxDesCycleCountAvg [max $cycleCountAvg]
set maxDesInitiationInterval [max $initiationIntervalAprx]

# Similarly, get the lowest throughput figure.
set minDesThroughputAvg  [min $throughputCycleCount]
set minDesThroughputIIAvg  [min $throughputIIAvg]

# ----------------------
# --- Display Result ---
# ----------------------
puts "cycleCountAvg:        $maxDesCycleCountAvg"
puts "throughputCycleCount: $minDesThroughputAvg MSa/s"

# ----------------------------
# --- Store in status file ---
# ----------------------------
set outFile [open $outStatus a]
puts $outFile "    cycleCountAvg:        $maxDesCycleCountAvg"
puts $outFile "    throughputCycleCount: $minDesThroughputAvg MSa/s"
close $outFile

