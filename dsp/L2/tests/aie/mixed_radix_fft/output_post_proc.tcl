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

set maxPtSize 4000

if { $::argc > 2} {
    set outputFile [lindex $argv 0]
    set fileDirpath [file dirname $outputFile]
    set window_vsize  [lindex $argv 1]
    set iterations  [lindex $argv 2]
    set maxPtSize  [lindex $argv 3]
    set inPtSizefile  [lindex $argv 4]
    set tt_data  [lindex $argv 5]
    set plio_width  [lindex $argv 6]

    puts "outputFile          = $outputFile"
    puts "window_vsize        = $window_vsize"
    puts "iterations          = $iterations"
    puts "maxPtSize           = $maxPtSize"
    puts "inPtSizefile        = $inPtSizefile"
    puts "plio_width          = $plio_width"
    puts "------------------------"

}

puts $tt_data
set dataSize 0
if {($tt_data eq "cint32" || $tt_data eq "cfloat")} {
    set dataSize 64
} elseif {($tt_data eq "cint16" || $tt_data eq "int32" || $tt_data eq "float" || $tt_data eq "cbfloat16")} {
    set dataSize 32
} elseif {($tt_data eq "int16" || $tt_data eq "uint16" || $tt_data eq "bfloat16" || $tt_data eq "cint8")} {
    set dataSize 16
} elseif {($tt_data eq "int8" || $tt_data eq "uint8")} {
    set dataSize 8
}
puts $dataSize
set samplesPerLine [expr {double($plio_width)/$dataSize}]
puts $samplesPerLine
set dataLineOfZeros 0
if {($tt_data eq "cint32" || $tt_data eq "cfloat")} {
    set dataLineOfZeros "0 0 "
} elseif {($tt_data eq "cint16")} {
    set dataLineOfZeros "0 0 0 0 "
} elseif {($tt_data eq "int32")} {
    set dataLineOfZeros "0 0 "
} elseif {($tt_data eq "float")} {
    set dataLineOfZeros "0 0 "
}
set inPtsFile [open $inPtSizefile r]
set tmpOutFile [open "tmpOutFile.txt" w]
set rawOutFile [open $outputFile r+]
set loopCount [expr $maxPtSize/$samplesPerLine]
set numFrames [expr $window_vsize/$maxPtSize]
set totIter [expr $numFrames * $iterations]
for {set i 1} {$i <= $iterations} {incr i} {
    gets $inPtsFile ptSize
    for {set fr 1} {$fr <= $numFrames} {incr fr 1} {
        for {set pt 1} {$pt <= $loopCount} {incr pt 1} {
            gets $rawOutFile outData
            if { $pt <= $ptSize/$samplesPerLine} {
                puts $tmpOutFile $outData
            } else {
                puts $tmpOutFile $dataLineOfZeros
            }
        }
    }
}


close $inPtsFile
close $tmpOutFile
close $rawOutFile

file delete $outputFile
file copy -force "tmpOutFile.txt" $outputFile
file delete "tmpOutFile.txt"

