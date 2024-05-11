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
set statusDir               [lindex $argv 0]
set logDir                  [lindex $argv 1]
set uutKernel               [lindex $argv 2]
set baseDir                 [lindex $argv 3]
set curDatetime             [lindex $argv 4]
set uutFileSuffix           [lindex $argv 5]

puts "statusDir: $statusDir"
puts "logDir: $logDir"
puts "uutKernel: $uutKernel"
puts "baseDir: $baseDir"
puts "curDatetime: $curDatetime"
puts "uutFileSuffix: $uutFileSuffix" 


#########################
### Batch Status File ###
#########################

# get current timestamp
set now [clock seconds]
# set timestr [clock format $now -format "%y%m%d-%H"]
set timestr "${curDatetime}00"

# open status file
set inFile [open $statusDir r]
set lines [split [read $inFile] "\n"]
close $inFile

# guard to ensure file exists 
set outStatus $baseDir/L2/tests/aie/batch_results/batch_status/batch_${timestr}.yaml
set outFile [open $outStatus a]
set uutKernelExists [ expr 1 - [catch {exec grep ${uutKernel} -c $outStatus}] ]
close $outFile

# open batch status file
set outFile [open $outStatus r+]

# do not re-print header if it already exists
if {$uutKernelExists == 0} {
    # open batch status file
    set outFile [open $outStatus a]
    puts $outFile "${uutKernel}:"
    puts $outFile "    status_${uutFileSuffix}:"
    foreach line $lines {
        puts $outFile "        ${line}"
    }
    close $outFile
} else {
    # open temporary batch status file (to overwrite original)
    set outFile [open $outStatus r+]
    set tempStatus $baseDir/L2/tests/aie/batch_results/batch_${timestr}-temp.yaml
    set tempFile [open $tempStatus w]

    while {[gets $outFile outLine]!=-1} {
        puts $outLine
        if {$outLine=="${uutKernel}:"} {
            puts $tempFile "${uutKernel}:"
            puts $tempFile "    status_${uutFileSuffix}:"
            foreach line $lines {
                puts $tempFile "        ${line}"
            }
        } else {
            puts $tempFile $outLine
        }
    }
    close $tempFile
    close $outFile

    # force replace original file
    file rename -force $tempStatus $outStatus
}


#####################
## Batch Log File ###
#####################
set outGrep $baseDir/L2/tests/aie/batch_results/batch_logs/batch_error_${timestr}.txt

set inGrep [open $logDir r]
set lines [split [read $inGrep] "\n"]
close $inGrep

set outGrepFile [open $outGrep a]
foreach line $lines {
    puts $outGrepFile "${line}"
}
close $outGrepFile