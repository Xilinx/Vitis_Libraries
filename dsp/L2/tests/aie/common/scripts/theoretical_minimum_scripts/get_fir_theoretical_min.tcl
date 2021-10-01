#
# Copyright 2021 Xilinx, Inc.
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
# ----------------
# --- Get Args ---
# ----------------
set dataType            [lindex $argv 0]
set coeffType           [lindex $argv 1]
set firLength           [lindex $argv 2]
set windowSize          [lindex $argv 3]
set cascLen             [lindex $argv 4]
set interpolateFactor   [lindex $argv 5]
set decimateFactor      [lindex $argv 6]
set symmetryFactor      [lindex $argv 7]
set outStatus           [lindex $argv 8]
set uutKernel           [lindex $argv 9]

# ----------------------------------------------
# --- Compute Data Type and Coeff Type Sizes ---
# ----------------------------------------------
set dataTypeSize 4
if {$dataType=="int16"} {
    set dataTypeSize 2
} elseif {$dataType=="cint16"} {
    set dataTypeSize 4
} elseif {$dataType=="int32"} {
    set dataTypeSize 4
} elseif {$dataType=="cint32"} {
    set dataTypeSize 8
} elseif {$dataType=="float"} {
    set dataTypeSize 4
} elseif {$dataType=="cfloat"} {
    set dataTypeSize 8
}

set coeffTypeSize 2
if {$coeffType=="int16"} {
    set coeffTypeSize 2
} elseif {$coeffType=="cint16"} {
    set coeffTypeSize 4
} elseif {$coeffType=="int32"} {
    set coeffTypeSize 4
} elseif {$coeffType=="cint32"} {
    set coeffTypeSize 8
} elseif {$coeffType=="float"} {
    set coeffTypeSize 4
} elseif {$coeffType=="cfloat"} {
    set coeffTypeSize 8
}

# -----------------------------------------------
# --- Adjust parameters depending on FIR type ---
# -----------------------------------------------
set hbCenterTap 0
set hbFactor 1
puts "adjusting parameters for $uutKernel"
# add halfband factor and center tap
if { $uutKernel == "fir_decimate_hb" || $uutKernel == "fir_interpolate_hb" } {
    set hbCenterTap 1
    set hbFactor 2
}
# remove symmetry factor for cfloat or float
if { $uutKernel == "fir_sr_sym" } {
    if { $dataType == "float" || $dataType == "cfloat" } {
        set symmetryFactor 1
    }
}

# ------------------------------
# --- Calculate theoreticals ---
# ------------------------------
# set up some constants
set kAieMacsPerCycle            [expr {128 / $dataTypeSize / $coeffTypeSize}]
set effectiveFirLength          [expr {(($firLength * $interpolateFactor) / ($cascLen * $symmetryFactor * $decimateFactor)) + $hbCenterTap}]
if { $effectiveFirLength == 0 } { 
    set effectiveFirLength 1
}
# Min cycle count (no overhead)
set minTheoryCycleCount         [expr {($windowSize / $interpolateFactor / $kAieMacsPerCycle / $hbFactor) * $effectiveFirLength}]
# Max theoretical throughput in MSa per second
set throughputTheoryMax  [expr {1000 * $windowSize / $minTheoryCycleCount}]

# ----------------------
# --- Display Result ---
# ----------------------
puts "cycleCountTheoryMin:  $minTheoryCycleCount"
puts "throughputTheoryMax:  $throughputTheoryMax MSa/s"

# ----------------------------
# --- Store in status file ---
# ----------------------------
set outFile [open $outStatus a]
puts $outFile "    cycleCountTheoryMin:  $minTheoryCycleCount"
puts $outFile "    throughputTheoryMax:  $throughputTheoryMax MSa/s"
close $outFile