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
set dataType            [lindex $argv 0]
set mixerMode           [lindex $argv 1]
set windowSize          [lindex $argv 2]
set ssr                 [lindex $argv 3]
set outStatus           [lindex $argv 4]
set uutKernel           [lindex $argv 5]

# ------------------------------------
# --- Compute Data Type Type Sizes ---
# ------------------------------------
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

# ------------------------------
# --- Calculate theoreticals ---
# ------------------------------
# compute mulsPerOutputV
set mulsPerOutputV -1
if { $mixerMode == 0 } {
    set mulsPerOutputV 1
} elseif { $mixerMode == 1 } {
    set mulsPerOutputV 2
} elseif { $mixerMode == 2 } {
    set mulsPerOutputV 4
}

set num_aie $ssr
# set up some constants
set kAieMacsPerCycle            [expr {(128 * $num_aie) / $dataTypeSize / $dataTypeSize}]
set effectiveFirLength          [expr {($mulsPerOutputV)}]
if { $effectiveFirLength == 0 } { 
    set effectiveFirLength 1
}
# Min cycle count (no overhead)
set minTheoryCycleCount         [expr {($windowSize / $kAieMacsPerCycle) * $effectiveFirLength}]

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