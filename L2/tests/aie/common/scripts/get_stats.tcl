
# Get args

set dataType            [lindex $argv 0]
set coeffType           [lindex $argv 1]
set firLength           [lindex $argv 2]
set windowSize          [lindex $argv 3]
set cascLen             [lindex $argv 4]
set interpolateFactor   [lindex $argv 5]
set decimateFactor      [lindex $argv 6]
set symmetryFactor      [lindex $argv 7]
set dualIp              [lindex $argv 8]
set useCoeffReload      [lindex $argv 9]
set numOutputs          [lindex $argv 10]
set outStatus           [lindex $argv 11]
set fileDir             [lindex $argv 12]
if { [llength $argv] > 13 } {
    set funcName           [lindex $argv 13]
} else {
    set funcName "filter"
}

if { [llength $argv] > 14 } {
    set numIter            [lindex $argv 14]
} else {
    set numIter           1
}

# Hack to get this script to work for FFT.
if { [string equal $funcName "kernelFFT"] } {
    set ptSize $windowSize
    set firLength [ expr int(log($ptSize)/log(2)) ]
}

# Get aiesimulator file names
set filterFunctionName $funcName
set fileNames [glob -directory $fileDir -- "profile_funct_*.xml"]
# Strings to helps extract cycle counts
set strLvl1  "    <function>"
set strLvl2  "        <function_name>$filterFunctionName"
set strLvl2main  "        <function_name>main"
set strLvl3  "        <function_and_descendants_time>"

set list initiationInterval
set list cycleCountTotal
set list cycleCountMin
set list cycleCountMax
set list cycleCountAvg
set list cycleCountTheoryMin
set list throughputTheoryMax
set list throughputAvg
set list throughputIIAvg


set x 0
set lineNo 0
set kernelStatsDone 0

# Parse xml files
    foreach fileName  $fileNames {
        if {$kernelStatsDone == 0} {
        # skip reading file if already found
        set inFile [open $fileName r]
                while {[gets $inFile line] != -1} {
                    incr lineNo
                    set strLine [string range $line 0 [string length $strLvl2main]-1]
                    if {![string compare $strLine $strLvl2main] } {
                        while {[gets $inFile line] != -1} {
                            incr lineNo
                            set strLine [string range $line 0 42]
                            if {![string compare $strLine $strLvl3]} {
                                puts "Function time match. File: $fileName. Line: $lineNo"
                                gets $inFile line
                                set mainCycleCountTotal    [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                                # Average out over number of iterations:
                                lappend initiationIntervalAprx    [expr {$mainCycleCountTotal  / $numIter}]
                                break
                            }
                        }
                    }
                    set strLine [string range $line 0 [string length $strLvl2]-1]
                    if {![string compare $strLine $strLvl2] } {
                        puts "Function name match. File: $fileName. Line: $lineNo. "

                        while {[gets $inFile line] != -1} {
                            incr lineNo
                            set strLine [string range $line 0 42]
                            if {![string compare $strLine $strLvl3]} {
                                puts "Function time match. File: $fileName. Line: $lineNo"
                                gets $inFile line
                                lappend cycleCountTotal    [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                                gets $inFile line
                                lappend cycleCountMin  [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                                gets $inFile line
                                lappend cycleCountMax  [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                                gets $inFile line
                                lappend cycleCountAvg  [lindex [split [lindex [split $line ">"] 1] "<"] 0]
                                break
                            }
                        }
                        incr x
                        break
                    }
                }
                puts "End of file reached. File: $fileName. "
        }
    }

close $inFile

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

# Calculate theoretical max throughput
# set up some constants
set kAieMacsPerCycle            [expr {128 / $dataTypeSize / $coeffTypeSize}]
set effectiveFirLengrh          [expr {($firLength) * $interpolateFactor / ($cascLen * $symmetryFactor * $decimateFactor)}]
if { $effectiveFirLengrh == 0 } { 
    set effectiveFirLengrh 1
}
# Min cycle count (no overhead)
set minTheoryCycleCount         [expr {($windowSize / $kAieMacsPerCycle) * $effectiveFirLengrh}]

# Max theoretical throughput in MSa per second
set throughputTheoryMax  [expr {1000 * $windowSize / $minTheoryCycleCount}]

for {set x 0} {$x<$cascLen} {incr x} {
    # Actual average throughput in MSa per second
    lappend throughputAvg  [expr {1000 * $windowSize / [lindex $cycleCountAvg $x]}]
    lappend throughputIIAvg  [expr {1000 * $windowSize / [lindex $initiationIntervalAprx $x]}]
}

# Display
puts "cycleCountTotal:      $cycleCountTotal"
puts "cycleCountMin:        $cycleCountMin"
puts "cycleCountMax:        $cycleCountMax"
puts "cycleCountAvg:        $cycleCountAvg"
puts "cycleCountTheoryMin:  $minTheoryCycleCount"
puts "throughputTheoryMax:  $throughputTheoryMax MSa/s"
puts "throughputAvg:        $throughputAvg MSa/s"
puts "initiationInterval:   $initiationIntervalAprx"
puts "throughpuInitIntAvg:  $throughputIIAvg MSa/s"


set outFile [open $outStatus a]
# Write to file
puts $outFile "    cycleCountTotal:      $cycleCountTotal"
puts $outFile "    cycleCountMin:        $cycleCountMin"
puts $outFile "    cycleCountMax:        $cycleCountMax"
puts $outFile "    cycleCountAvg:        $cycleCountAvg"
puts $outFile "    cycleCountTheoryMin:  $minTheoryCycleCount"
puts $outFile "    throughputTheoryMax:  $throughputTheoryMax MSa/s"
puts $outFile "    throughputAvg:        $throughputAvg MSa/s"
puts $outFile "    initiationInterval:   $initiationIntervalAprx"
puts $outFile "    throughpuInitIntAvg:  $throughputIIAvg MSa/s"


close $outFile



#  (c) Copyright 2014 - 2020 Xilinx, Inc. All rights reserved.
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
