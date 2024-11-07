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
For generating random stimulus for data files.
tclsh gen_input.tcl <filename> <numSamples> <iterations> \[<seed>\] \[<dataStimType>\]

";
if { [lsearch $argv "-h"] != -1 } {
    puts $usage
    exit 0
}

puts "\n"
puts "Align the output of Stream data Starts: \n "
# defaults
# File Directory and file type to search
set fileDirpath "./data"
set input_filename "$fileDirpath/uut_output.txt"
set tp_phases     1
set tp_casc_len   1
set tp_Sig_len    32
set tp_num_iter   1
set tp_API        1

if { $::argc >= 2} {
set inp_ref_file [lindex $argv 0]
set inp_uut_file [lindex $argv 1]
set tp_phases  [lindex $argv 2]
set tp_casc_len [lindex $argv 3]
set tp_window_size [lindex $argv 4]
set tp_Sig_len [lindex $argv 5]
set tp_num_iter [lindex $argv 6]
set tp_API     [lindex $argv 7]
}

puts "inp_ref_file      = $inp_ref_file"
puts "inp_uut_file      = $inp_uut_file"
puts "tp_phases         = $tp_phases"
puts "tp_casc_len       = $tp_casc_len"
puts "tp_window_size    = $tp_window_size"
puts "tp_Sig_len        = $tp_Sig_len"
puts "tp_num_iter       = $tp_num_iter"
puts "tp_API            = $tp_API"

if {$tp_API == 1} {
    set streams_per_core 1
    set macspercore 1
    set delay 0
    set discordedSamples_count 0
    set Lanes 4
    set Points 2
    
    proc ceil {x y} {
       set RetVal [expr ((($x+$y)-1)/ $y) ]
       return $RetVal
    }
    
    proc round {x y} {
       set RetVal [ ([expr ($x%$y)] > [expr (($y+1)/2)]) ? [expr ((($x+$y)-1)/ $y)] : [expr ($x/$y)]]
       return $RetVal
    }
    
    proc floor {x y} {
       puts "x = $x"
       puts "y = $y"
       set RetVal [([expr ($x)] >= 0) ? [expr ($x/$y)] : [ceil $x $y ]]
       return $RetVal
    }
    
    proc rem {x y} {
        set RetVal [expr ($x%$y)]
       return $RetVal
    }
    
    set offset ([expr ($tp_phases * $Lanes * $Points)])
    if { $tp_Sig_len > ([expr ($offset/2)])} {
       set streams_per_core 1
    } else {
       set streams_per_core 2
    }
    puts "streams_per_core = $streams_per_core"
    
    set offset2 [expr ($offset* $tp_casc_len)]
    set macspercore  [ceil $tp_Sig_len $offset2 ]
    set phase_incr [expr ($tp_phases/$streams_per_core)]
    set casclen_offset  [expr ($tp_casc_len-1)]
    set phaseincr_offset [expr ($phase_incr-1)]
    
    puts "offset2 = $offset2"
    puts "macspercore = $macspercore"
    puts "phase_incr = $phase_incr"
    puts "casclen_offset = $casclen_offset"
    puts "phaseincr_offset = $phaseincr_offset"
    
    set term_1 [expr ([expr ((8*$macspercore*$Points/2)-3)]*[expr int(floor($casclen_offset/$phase_incr))])]
    set term_2 [expr (3*[expr ([expr int(floor($casclen_offset/$phase_incr))]-1)]*$phaseincr_offset)]
    set term_3 [expr (3*[expr ($casclen_offset%$phase_incr)])]
    
    set delay [expr ([expr ([expr ($term_1 - $term_2 - $term_3)]*$phase_incr)]-1)]
                         
    set delay_offset [expr (3*$tp_phases*$phaseincr_offset)]
    set samplecount_UUT [expr (($tp_Sig_len-$delay-1)+$delay_offset)]
    set samplecount_REF [expr ($delay_offset)]
    
    puts "delay = $delay"
    puts "delay_offset = $delay_offset"
    puts "samplecount_UUT = $samplecount_UUT"
    puts "samplecount_REF = $samplecount_REF"
    
    proc align_outdata {input_filename len_window len_of_sig sample_count delay_offset num_iter} {
        #open Input file to read 
        set fd [open $input_filename r]
        set indata [read $fd]
        close $fd
        
        set indata [string trim $indata]
        set elements [split $indata " "]
        
        for {set iter_nr 1} {$iter_nr <= [expr ($num_iter)]} {incr iter_nr} {
            set startIndex [expr ($sample_count*2)+([expr ($iter_nr - 1)]*$len_window)]
            #set endIndex [expr {[expr ([llength $elements]-1)] - [expr (($len_of_sig +$delay_offset - [expr ($sample_count)])*2)]}]
            set endIndex [expr {[expr (($len_window * $iter_nr)-1)] - [expr (($len_of_sig +$delay_offset - [expr ($sample_count)])*2)]}]
            
            puts "num_iter = $num_iter"
            puts "startIndex = $startIndex"
            puts "endIndex = $endIndex"
            puts " "

            lappend newelements {*}[lrange $elements $startIndex $endIndex]
        }
        
        set newindata [join $newelements " "]
        set newindata [string trim $newindata]
        
        set fd [open $input_filename w]
        puts -nonewline $fd $newindata
        close $fd
    }
    puts "Align the data for REF:   "
    align_outdata $inp_ref_file $tp_window_size $tp_Sig_len $samplecount_REF $delay_offset $tp_num_iter
    puts "Align the data for UUT:   "
    align_outdata $inp_uut_file $tp_window_size $tp_Sig_len $samplecount_UUT $delay_offset $tp_num_iter
      
    puts " =========================================================================="
    puts " Alignment of Stream based output Done for $inp_ref_file and $inp_uut_file " 
    puts " =========================================================================="
    puts " "
} else {
    puts " ==================================================================================="
    puts " No Need of Alignment for window bases output for $inp_ref_file and $inp_uut_file   " 
    puts " ==================================================================================="
    puts " "
}
