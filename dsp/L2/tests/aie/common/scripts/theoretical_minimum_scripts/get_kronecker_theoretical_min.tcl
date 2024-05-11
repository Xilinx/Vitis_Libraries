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
set dataType_A          [lindex $argv 0]
set dataType_B          [lindex $argv 1]
set windowSize          [lindex $argv 2]
set outStatus           [lindex $argv 3]
set uutKernel           [lindex $argv 4]

# widget api cast parameters
if { [llength $argv] > 5 } {
    set Api               [lindex $argv 5]
} else {
    set Api 0
}

puts "uutKernel: $uutKernel"


# -------------------------------
# --- Compute Data Type Sizes ---
# -------------------------------
if {$dataType_A=="int16"} {
    set dataTypeSize_A 2
} elseif {$dataType_A=="cint16"} {
    set dataTypeSize_A 4
} elseif {$dataType_A=="int32"} {
    set dataTypeSize_A 4
} elseif {$dataType_A=="cint32"} {
    set dataTypeSize_A 8
} elseif {$dataType_A=="float"} {
    set dataTypeSize_A 4
} elseif {$dataType_A=="cfloat"} {
    set dataTypeSize_A 8
}
if {$dataType_B=="int16"} {
    set dataTypeSize_B 2
} elseif {$dataType_B=="cint16"} {
    set dataTypeSize_B 4
} elseif {$dataType_B=="int32"} {
    set dataTypeSize_B 4
} elseif {$dataType_B=="cint32"} {
    set dataTypeSize_B 8
} elseif {$dataType_B=="float"} {
    set dataTypeSize_B 4
} elseif {$dataType_B=="cfloat"} {
    set dataTypeSize_B 8
}

if {($dataType_A=="cint32") || ($dataType_A=="cint32")} {
    set dataTypeSize_out 8
} elseif {($dataType_A=="int16") && ($dataType_A=="int16")} {
    set dataTypeSize_out 2
} elseif {($dataType_A=="int16") && ($dataType_A=="int32")} {
    set dataTypeSize_out 4
} elseif {($dataType_A=="int16") && ($dataType_A=="cint16")} {
    set dataTypeSize_out 4
} elseif {($dataType_A=="int32") && ($dataType_A=="int16")} {
    set dataTypeSize_out 4
} elseif {($dataType_A=="int32") && ($dataType_A=="int32")} {
    set dataTypeSize_out 4
} elseif {($dataType_A=="int32") && ($dataType_A=="cint16")} {
    set dataTypeSize_out 8
} elseif {($dataType_A=="cint16") && ($dataType_A=="int16")} {
    set dataTypeSize_out 4
} elseif {($dataType_A=="cint16") && ($dataType_A=="int32")} {
    set dataTypeSize_out 8
} elseif {($dataType_A=="cint16") && ($dataType_A=="cint16")} {
    set dataTypeSize_out 4
} elseif {($dataType_A=="cfloat") || ($dataType_A=="cfloat")} {
    set dataTypeSize_out 8
} else {
    set dataTypeSize_out 4
}

puts "dataTypeSize_A: $dataTypeSize_A"
puts "dataTypeSize_B: $dataTypeSize_B"
puts "dataTypeSize_out: $dataTypeSize_out"


# ------------------------------
# --- Calculate theoreticals ---
# ------------------------------

set kAieMacsPerCyclePerKernel   [expr {128 / $dataTypeSize_A / $dataTypeSize_B}]


if {($Api==0)} {
    set kVecPerCalc [expr {32/$dataTypeSize_out}]        
} else {
    set kVecPerCalc [expr {16/$dataTypeSize_out}] 
}

# Min cycle count (no overhead)
set minTheoryCycleCount  [expr {$windowSize / min($kVecPerCalc, $kAieMacsPerCyclePerKernel)}]

# compute throughput
set throughputTheoryMax  [expr {1000 * $minTheoryCycleCount}]

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
