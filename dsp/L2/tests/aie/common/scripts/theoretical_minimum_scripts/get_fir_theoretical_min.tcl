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
set dataType            [lindex $argv 0]
set coeffType           [lindex $argv 1]
set firLength           [lindex $argv 2]
set windowSize          [lindex $argv 3]
set cascLen             [lindex $argv 4]
set interpolateFactor   [lindex $argv 5]
set decimateFactor      [lindex $argv 6]
set symmetryFactor      [lindex $argv 7]
set ssr                 [lindex $argv 8]
set paraInterpPoly      [lindex $argv 9]
set paraDeciPoly        [lindex $argv 10]
set outStatus           [lindex $argv 11]
set uutKernel           [lindex $argv 12]

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

set oddSym 0
if { ( $symmetryFactor == 2 ) && ( $firLength % 2 == 1 ) } {
    # adjustment to ceil properly when dividing by symFactor
    set oddSym 1
}


# ------------------------------
# --- Calculate theoreticals ---
# ------------------------------
# set up some constants
set kAieMacsPerCyclePerKernel   [expr {128 / $dataTypeSize / $coeffTypeSize}]
set numKernels                  [expr {$ssr * $ssr * $cascLen * $paraInterpPoly * $paraDeciPoly}]
set kAieMacsPerCycle            [expr { $kAieMacsPerCyclePerKernel * $numKernels}] ;

set numberOfOutputs [expr ($windowSize * $interpolateFactor) / $decimateFactor ] ;

# output double because it's likely that numOutputs will turn this back into an int for totalMacs.
# We have fractions here when halfband interpolator is used - for a fir len of 15 you have 2.5 macs/output .
set numberOfMacsPerOutput [expr { double( ( $firLength + $oddSym / ($symmetryFactor * $hbFactor) ) + $hbCenterTap) /  double($interpolateFactor) }]

if { $numberOfMacsPerOutput == 0 } {
    set numberOfMacsPerOutput 1
}

# force it back to int any fractional macs (unlikely) rounded up
set totalMacs [expr { int(ceil($numberOfOutputs * $numberOfMacsPerOutput)) }]

set minTheoryCycleCount [expr { $totalMacs / $kAieMacsPerCycle }]

if { $minTheoryCycleCount == 0 } {
    set minTheoryCycleCount 1
}

# Min cycle count (no overhead)
# Max theoretical throughput in MSa per second
set clk_rate 1000000000; #1Ghz - although might change with overclocking to 1.2GHz
set megaCyclesPerSecond [expr {$clk_rate / 1000000}]
set throughputTheoryMax  [expr { ($megaCyclesPerSecond * $windowSize) / $minTheoryCycleCount}]


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
