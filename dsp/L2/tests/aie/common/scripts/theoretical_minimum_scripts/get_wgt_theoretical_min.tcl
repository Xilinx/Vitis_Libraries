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
set windowSize          [lindex $argv 1]
set outStatus           [lindex $argv 2]
set uutKernel           [lindex $argv 3]

# widget api cast parameters
if { [llength $argv] > 4 } {
    set inApi               [lindex $argv 4]
} else {
    set inApi 0
}
if { [llength $argv] > 5 } {
    set outApi              [lindex $argv 5]
} else {
    set outApi 0
}
if { [llength $argv] > 6 } {
    set numOutputClones     [lindex $argv 6]
} else {
    set numOutputClones 2
}
if { [llength $argv] > 7 } {
    set numInputs           [lindex $argv 7]
} else {
    set numInputs 1
}

puts "uutKernel: $uutKernel"
puts "inApi: $inApi"
puts "outApi: $outApi"
puts "numOutputClones: $numOutputClones"
puts "numInputs: $numInputs"


# -------------------------------
# --- Compute Data Type Sizes ---
# -------------------------------
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

puts "dataTypeSize: $dataTypeSize"

# ------------------------------
# --- Calculate theoreticals ---
# ------------------------------
# Widget Api Cast
if {$uutKernel=="widget_api_cast"} {
    if {($inApi==0) && ($outApi==0)} {
        set minTheoryCycleCount     [expr {$windowSize / (32/$dataTypeSize) / $numOutputClones}]
    } elseif {($inApi==1) && ($outApi==0)} {
        set minTheoryCycleCount     [expr {$windowSize / (4/$dataTypeSize) / $numInputs}]
    } elseif {($inApi==0) && ($outApi==1)} {
        set minTheoryCycleCount     [expr {$windowSize / (4/$dataTypeSize) / $numOutputClones}]
    } else {
        set minTheoryCycleCount     [expr {$windowSize / (4/$dataTypeSize) / min($numOutputClones, $numInputs)}]
    }

# Widget Real to Complex
} elseif {$uutKernel=="widget_real2complex"} {
    set baseTypeSize 2
    if {($dataType=="float") || ($dataType=="cfloat") || ($dataType=="int32") || ($dataType=="cint32")} {
        set baseTypeSize 4
    }
    set vSize [expr {16 / $baseTypeSize}]
    set minTheoryCycleCount     [expr {$windowSize / $vSize}]

# FFT Window
} elseif {$uutKernel=="fft_window"} {
    set baseTypeSize 2
    if {($dataType=="cfloat") || ($dataType=="cint32")} {
        set baseTypeSize 4
    }
    set vSize [expr {16 / $baseTypeSize}]
    set minTheoryCycleCount     [expr {$windowSize / $vSize}]
}

# compute throughput
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
