# Compare files containing numbers, line by line, value by value.


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
    # any entry sets absolute tolerance mode on
    set toleranceMode               [lindex $argv 4]
    puts  "toleranceMode = $toleranceMode" 
        if {($toleranceMode == "abs")|| ($toleranceMode == "ABS") } {
            set toleranceMode  abs
            # Absolute tolerance Mode
           puts "Using Absolute Tolerance Mode"         
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



proc isSame {a b tolerance toleranceMode} {
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
        if {$diffVal <= $tolerance} {
            set res 1
        } else {
            set res 0
        }
    }
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

# Compare line by line, until EoF.
while {$fexist1 && $fexist2 && [gets $inFile1 line1] != -1} {
    incr lineNo
    if {[gets $inFile2 line2] != -1} {
        set valList1 [split $line1 " "]
        set valList2 [split $line2 " "]
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
        # Eof reached on file2,
        set fileLengthMismatch 1
        break
    }
}


if {!$fexist1} {
    # File not found
    puts "Error: File not found: $fileName1 "
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "Error: File not found: $fileName1 "
    close $outFile
} elseif {!$fexist2} {
    # File not found
    puts "Error: File not found: $fileName2 "
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "Error: File not found: $fileName2 "
    close $outFile
} elseif {$lineNo == 0} {
    # Empty file
    puts "Error: File empty: $fileName1"
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "Error: File empty: $fileName1"
    close $outFile
} elseif {$fileLengthMismatch != 0} {
    # Empty file
    puts "Error: File length mismatch. Insufficient data in: $fileName2"
    set outFile [open $fileNameOut a]
    # Write to file
    puts $outFile "Error: File length mismatch. Insufficient data in: $fileName2"
    close $outFile
} elseif {$fileMismatch != 0} {
    # Diffs are showing mismatches
    puts "WARNING: Compared files differ. Differences found: $fileDiffsFound"
} else {
    # Good. Put a phrase to a file, which mimics the diff command behaviour.
    set outFile [open $fileNameOut a]
    puts $outFile "Files are identical."
    close $outFile
    puts "INFO: Compared files match."
}



#  (c) Copyright 2020 Xilinx, Inc. All rights reserved.
#
#    This file contains confidential and proprietary information
#    of Xilinx, Inc. and is protected under U.S. and
#    international copyright and other intellectual property
#    laws.
#
#    DISCLAIMER
#    This disclaimer is not a license and does not grant any
#    rights to the materials distributed herewith. Except as
#    otherwise provided in a valid license issued to you by
#    Xilinx, and to the maximum extent permitted by applicable
#    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
#    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
#    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
#    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
#    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
#    (2) Xilinx shall not be liable (whether in contract or tort,
#    including negligence, or under any other theory of
#    liability) for any loss or damage of any kind or nature
#    related to, arising under or in connection with these
#    materials, including for any direct, or any indirect,
#    special, incidental, or consequential loss or damage
#    (including loss of data, profits, goodwill, or any type of
#    loss or damage suffered as a result of any action brought
#    by a third party) even if such damage or loss was
#    reasonably foreseeable or Xilinx had been advised of the
#    possibility of the same.
#
#    CRITICAL APPLICATIONS
#    Xilinx products are not designed or intended to be fail-
#    safe, or for use in any application requiring fail-safe
#    performance, such as life-support or safety devices or
#    systems, Class III medical devices, nuclear facilities,
#    applications related to the deployment of airbags, or any
#    other applications that could lead to death, personal
#    injury, or severe property or environmental damage
#    (individually and collectively, "Critical
#    Applications"). Customer assumes the sole risk and
#    liability of any use of Xilinx products in Critical
#    Applications, subject only to applicable laws and
#    regulations governing limitations on product liability.
#
#    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
#    PART OF THIS FILE AT ALL TIMES.
