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
puts "ReArrangeInData Starts: \n "
# defaults
# File Directory and file type to search
set fileDirpath "./data"
set input_filename "$fileDirpath/inData_F.txt"
set tt_data "float" ;
set tp_dim 4 ;
set tp_fixed_dim 4
set samplesPerFrame 256
set using_plio_class 1 ;# default (backwards compatible)

if { $::argc >= 3} {
set input_filename [lindex $argv 0]
set tp_data  [lindex $argv 1]
set tp_dim  [lindex $argv 2]
set tp_fixed_dim [lindex $argv 3]
}


puts "input_filename = $input_filename"
puts "tt_data         = $tt_data"
puts "tp_dim         = $tp_dim"
puts "tp_fixed_dim   = $tp_fixed_dim"

# Number of files to read based on given number of phases
set fileHandles {}
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
set fdIn [open $input_filename r]
set content [read $fdIn]
close $fdIn

# Split the content into a list of lines
set lines [split $content "\n"]

# Initialize an empty list to store the modified lines
set modified_lines {}

# Calculate the number of zeros to append
set zeros_to_append [expr {$tp_fixed_dim - $tp_dim}]

# Initialize a list to store all elements
set all_elements {}

# Collect all elements from the lines
    foreach line $lines {
        if {[string trim $line] ne ""} {
            set elements [split $line " "]
            foreach element $elements {
                set element [string trim $element "{}"]
                if {$element ne ""} {
                    lappend all_elements $element
                }
            }
        }
    }

# Initialize an empty list to store the modified elements
set modified_elements {}

# Iterate over all elements and append zeros based on the given dimension and fixed dimension
set count 0
foreach element $all_elements {
    lappend modified_elements $element
    incr count
    if {($count % $tp_dim) == 0} {
        for {set i 0} {$i < $zeros_to_append} {incr i} {
            lappend modified_elements 0.00
        }
        set count 0
    }
}

# Split the modified elements into lines of 2 elements each
set temp_line {}
set temp_count 0
foreach element $modified_elements {
    lappend temp_line $element
    incr temp_count
    if {$temp_count == $dataPartsPerLine} {
        lappend modified_lines [join $temp_line " "]
        set temp_line {}
        set temp_count 0
    }
}

#If there are remaining elements, add them as a new line
if {[llength $temp_line] > 0} {
    lappend modified_lines [join $temp_line " "]
}

# Join the modified lines into a string
set modified_content [join $modified_lines "\n"]

# Write the modified content back to the file
set fdOut [open $input_filename w]
puts -nonewline $fdOut $modified_content
close $fdOut

puts "ReArrangeInData Ends: \n"
puts "\n"