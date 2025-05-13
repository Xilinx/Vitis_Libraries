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
set tDataA              [lindex $argv 0]
set tDataB              [lindex $argv 1]
set numMacsPerKernel    [lindex $argv 2]
set pCascLen            [lindex $argv 3]
set outStatus           [lindex $argv 4]
set uutKernel           [lindex $argv 5]

# -------------------------------
# --- Compute Data Type Sizes ---
# -------------------------------
set tDataASize 4
if {$tDataA=="int16"} {
    set tDataASize 2
} elseif {$tDataA=="cint16"} {
    set tDataASize 4
} elseif {$tDataA=="int32"} {
    set tDataASize 4
} elseif {$tDataA=="cint32"} {
    set tDataASize 8
} elseif {$tDataA=="float"} {
    set tDataASize 4
} elseif {$tDataA=="cfloat"} {
    set tDataASize 8
}

set tDataBSize 2
if {$tDataB=="int16"} {
    set tDataBSize 2
} elseif {$tDataB=="cint16"} {
    set tDataBSize 4
} elseif {$tDataB=="int32"} {
    set tDataBSize 4
} elseif {$tDataB=="cint32"} {
    set tDataBSize 8
} elseif {$tDataB=="float"} {
    set tDataBSize 4
} elseif {$tDataB=="cfloat"} {
    set tDataBSize 8
}

# ------------------------------
# --- Calculate theoreticals ---
# ------------------------------
# set up some constants
set kAieMacsPerCycle            [expr {128 / $tDataASize / $tDataBSize}]
set effectiveFirLength          [expr {(1 / $pCascLen)}]
if { $effectiveFirLength == 0 } {
    set effectiveFirLength 1
}
# Min cycle count (no overhead)
set minTheoryCycleCount         [expr {($numMacsPerKernel / $kAieMacsPerCycle) * $effectiveFirLength}]

# Max theoretical throughput in MSa per second
set throughputTheoryMax  [expr {1000 * $numMacsPerKernel / $minTheoryCycleCount}]

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