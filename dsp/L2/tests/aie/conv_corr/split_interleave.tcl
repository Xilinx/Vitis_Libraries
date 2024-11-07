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
set usage "
For generating single file by deinterleave the data from the different input files based on Num_of_Phases.
tclsh streams_to_window.tcl <filename> <numSamples> <iterations> \[<seed>\] \[<dataStimType>\]



";
if { [lsearch $argv "-h"] != -1 } {
    puts $usage
    exit 0
}
puts "\n"
puts "interleave Starts: \n "
# defaults
# File Directory and file type to search
set fileDirpath "./data"
set input_filename "$fileDirpath/inData_F.txt"
set tt_data "cint16" ;# sensible default of complex data
set tp_api 0 ;#  for high throughput fft (planned)
set tp_phases 1
set samplesPerFrame 256
set using_plio_class 1 ;# default (backwards compatible)

if { $::argc >= 2} {
set input_filename [lindex $argv 0]
set tt_data  [lindex $argv 1]
set tp_phases [lindex $argv 2]
}


puts "input_filename    = $input_filename"
puts "tp_phases         = $tp_phases"
puts "tt_data           = $tt_data"

# Number of files to read based on given number of phases
set fileHandles {}
set filecount $tp_phases
file mkdir $fileDirpath




#ADF::PLIO class expects data in 32-bits per text line, which for cint32 & cfloat is half a sample per line.
if {$using_plio_class == 1 && ($tt_data eq "cint32" || $tt_data eq "cfloat")} {
    set samplesPerFrame [expr ($samplesPerFrame) * 2]
    }
set samplesPerLine 2
if {($tt_data eq "int16" || $tt_data eq "uint16" || $tt_data eq "bfloat16")} {
    # int16s are organized in 2 samplesPerLine
    set samplesPerLine 2
}

if {($tt_data eq "int8" || $tt_data eq "uint8" || $tt_data eq "cint8")} {
    # int8 values are organized in 4 samplesPerLine
    set samplesPerLine 4
}

#ADF::PLIO expects data in 32-bits per text line, which for cint16 and int16 is 2 samplesPerFrame/dataPartsPerLine per line
set dataPartsPerLine 2
if {$using_plio_class == 0} {
    if {$tt_data eq "cint16" || $tt_data eq "int16" || $tt_data eq "cint32" || $tt_data eq "cfloat" || $tt_data eq "bfloat16"} {
        set dataPartsPerLine 2
    }
     if {$tt_data eq "cint8" || $tt_data eq "int8" || $tt_data eq "uint8"} {
        set dataPartsPerLine 4
    }
} else { #PLIO
    if {$tt_data eq "cint16" || $tt_data eq "int16" || $tt_data eq "uint16" || $tt_data eq "bfloat16" } {
        set dataPartsPerLine 4
    }
    if {$tt_data eq "cint8" || $tt_data eq "int8" || $tt_data eq "uint8"} {
        set dataPartsPerLine 8
    }
}

#open Input file to read 
set fd [open $input_filename r]

# Open all files and store the file handles in a list
for {set phaseIndex 0} {$phaseIndex < $filecount} {incr phaseIndex} {
    set outfilename "$fileDirpath/inData_F_$phaseIndex.txt"
    set output_file [open $outfilename w]
    lappend fileHandles $output_file
}

# Read elements from input file and write those into output file based on phases
set filenum 0
set newline 0
set elemcount 0
while {[gets $fd line] >= 0} {
    set count 0
    foreach element $line {
       set output_file [lindex $fileHandles [expr {$filenum % $filecount }]]
       puts -nonewline $output_file "$element "
       
       if {($count == 1) || ($count == 3)} {
          incr filenum
       } 
       incr count
    }    
}

# close all files
close $fd
foreach fds $fileHandles {
    close $fds
}

for {set phaseIndex 0} {$phaseIndex < $filecount} {incr phaseIndex} {
    set outfilename "$fileDirpath/inData_F_$phaseIndex.txt"
    set fdout [open $outfilename r]
    set singleline [gets $fdout]
    close $fdout
    
    set outlines ""
    set presentline ""
    set count 0
    set singleline [string trim $singleline]
    set elements [split $singleline " "]
    
    foreach element $elements {
        if {$count == $dataPartsPerLine} {
            append outlines "\n"
            set count 0
        }
        
        append outlines "$element "
        incr count
    }
    
    append outlines "\n"
    
    #write the output lines back to the same file
    set fdin [open $outfilename w]
    puts -nonewline $fdin $outlines
    close $fdin
}    

puts "interleave Ends: \n"
puts "\n"




