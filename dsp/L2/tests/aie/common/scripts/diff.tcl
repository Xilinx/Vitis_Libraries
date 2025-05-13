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
set usage "
Compare files containing numbers, line by line, value by value.
Print found differences into a <status> file.
Specify acceptable error with optional \[<tolerance>\] - default: 0.001.
Select between percent (relative) and abs (absolute) \[<toleranceMode>\] -  default: percent

Usage:
tclsh diff.tcl <filename1> <filename2> <status> \[<tolerance>\] \[<toleranceMode>\]

";
if { [lsearch $argv "-h"] != -1 } {
    puts $usage
    exit 0
}


# Get args
set fileName1             [lindex $argv 0]
set fileName2             [lindex $argv 1]
set fileNameOut           [lindex $argv 2]
if { $::argc >= 4} {
    set tolerance         [lindex $argv 3]
} else {
    # Default tolerenace
    set tolerance         0.001
}

if { $::argc >= 5} {
    set cc_tolerance         [lindex $argv 4]
} else {
    # Default tolerenace
    set cc_tolerance         0.001
}
if { $::argc >= 6} {
    # any entry sets absolute tolerance mode on
    set toleranceMode               [lindex $argv 5]
    puts  "toleranceMode = $toleranceMode"
    if {($toleranceMode == "abs")|| ($toleranceMode == "ABS") } {
        set toleranceMode  abs
        # Absolute tolerance Mode
        puts "Using Absolute Tolerance Mode"
        puts  "tolerance = $tolerance"
    } else {
        # Percentage tolerance Mode
        puts "Using Percentage Tolerance Mode"
    }
} else {
    # Default abs off ( i.e. percentage tolerance mode on )
    set toleranceMode         percent
    puts  "toleranceMode = $toleranceMode"
    # Percentage tolerance Mode
    puts "Using Percentage Tolerance Mode"
}

# Validate tolerance for percentage mode
if {($toleranceMode == "percent" || $toleranceMode == "PERCENT") && $tolerance > 0.5} {
    puts "error: Tolerance for percentage mode should not exceed 0.5 (50%)."
    exit 1
}

# global variable to track max difference and zero samples
set maxDiffVal 0
set sampleNum 0
set zeroSamples 0

proc isSame {a b tolerance toleranceMode} {
    variable maxDiffVal
    variable sampleNum
    variable zeroSamples

    if {($a == 0.0 && $b == 0.0)} {
        incr zeroSamples
    }

    if {(($a == 0.0) && ($toleranceMode != "abs"))} {
        # Avoid division by 0
        if {$b == 0.0} {
            set res 1
        } else {
            set res 0
        }
    } else {
        if {($toleranceMode == "abs")} {
            # Absolute tolerance
            set diffVal [expr {abs(($a-$b))}]
        } else {
            # Percentage tolerance
            set diffVal [expr {abs(((double(($a-$b))) / double($a)))}]
        }
        if {($maxDiffVal < $diffVal)} {
            set maxDiffVal $diffVal
        }
        if {$diffVal <= $tolerance} {
            set res 1
        } else {
            set res 0
        }
    }
    incr sampleNum
    return $res
}

set outFile [open $fileNameOut w]
# Write header to file
puts $outFile "Comparing files: "
puts $outFile "File: $fileName1:"
puts $outFile "File: $fileName2:"
puts $outFile "Tolerance ($toleranceMode): $tolerance: "
close $outFile

set fileMismatch 0
set fileLengthMismatch 0
set fileDiffsFound 0
set fileMatchesFound 0
set lineNo 0

# Open files if exist
set fexist1 [file exist $fileName1]
set fexist2 [file exist $fileName2]
if {$fexist1} {
    set inFile1 [open $fileName1 r]
}
if {$fexist2} {
    set inFile2 [open $fileName2 r]
}

if {$fexist1 && $fexist2} {
    # Compare line by line, until EoF.
    while {[gets $inFile1 line1] != -1} {
        incr lineNo
        if {[gets $inFile2 line2] != -1} {
            set valList1 [split $line1 " "]
            set valList2 [split $line2 " "]
            # Check if the number of numbers in each line is the same
            if {[llength $valList1] != [llength $valList2]} {
                set outFile [open $fileNameOut a]
                puts "Error: Line $lineNo has different number of samples/parts."
                puts $outFile "Error: Line $lineNo has different number of samples/parts."
                # Write line differences to the differences file
                puts $outFile "Line no: $lineNo:     $line1"
                puts $outFile "Line no: $lineNo:     $line2"
                close $outFile
                exit 1
            }
            set indexList2 0
            set printLines 0
            foreach val1  $valList1 {
                # Assume that each files have same number of arguments
                # skip empty spaces at the end of the line
                if {($val1!="")} {
                    if {[isSame $val1  [lindex $valList2 $indexList2] $tolerance $toleranceMode] } {
                        # Good, move on
                        incr fileMatchesFound
                    } else {
                        # Bad, set out flag to print out diff lines to diff file.
                        set printLines 1
                        # and set the comparison result
                        set fileMismatch 1
                        incr fileDiffsFound
                    }
                }
                incr indexList2
            }
            if {$printLines == 1} {
                set outFile [open $fileNameOut a]
                # Write to file
                puts $outFile "Line no: $lineNo:     $line1"
                puts $outFile "Line no: $lineNo:     $line2"
                close $outFile
            }
        } else {
            # inFile2 too short
            set fileLengthMismatch 1
        }
    }
    # Error if other file still has data left.
    if  {[gets $inFile2 line2] != -1} {
        # inFile2 too long
        set fileLengthMismatch 1
    }
}

# Deal with catastrophic cancellations for floats
set fileAllSamples  [expr ($fileDiffsFound + $fileMatchesFound)]
if {$fileAllSamples == 0.0} {
    # Avoid division by 0
    set fileErrRatio 1
} else {
    set fileErrRatio  [expr {(double($fileDiffsFound) / double($fileAllSamples))}]
}
if {$fileErrRatio > $cc_tolerance} {
    set fileMismatch 1
} else {
    set fileMismatch 0
}
puts $fileMismatch

if {!$fexist1} {
    # File not found
    puts "error: File not found: $fileName1 "
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "error: File not found: $fileName1 "
    close $outFile
} elseif {!$fexist2} {
    # File not found
    puts "error: File not found: $fileName2 "
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "error: File not found: $fileName2 "
    close $outFile
} elseif {$fileLengthMismatch != 0} {
    # Empty file
    puts "error: File length mismatch."
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "error: File length mismatch. "
    close $outFile
} elseif {$lineNo == 0} {
    # Empty file
    puts "error: File empty: $fileName1"
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "error: File empty: $fileName1"
    close $outFile
} elseif {$zeroSamples == $fileAllSamples} {
    # All samples are zero
    puts "warning: All samples are zero in both files. This may indicate an issue."
    set outFile [open $fileNameOut a]
    puts $outFile "warning: All samples are zero in both files. This may indicate an issue."
    close $outFile
} elseif {$fileMismatch != 0} {
    # Diffs are showing mismatches
    puts "error: Compared files differ. Differences found: $fileDiffsFound. Max Difference: $maxDiffVal"
} else {
    # Good. Put a phrase to a file, which mimics the diff command behaviour.
    set outFile [open $fileNameOut a]
    puts $outFile "Files are identical, i.e. compared data samples are within defined tolerance bounds. "
    close $outFile
    puts "INFO: Compared files match, i.e. compared data samples are within defined tolerance bounds. "
}
exit 0