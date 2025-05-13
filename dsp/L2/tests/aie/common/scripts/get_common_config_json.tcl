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
set outConfig           [lindex $argv 0]
set fileDir             [lindex $argv 1]
set libElement          [lindex $argv 2]
puts "$outConfig $fileDir $libElement"
#todo, add to this to cover the full spectrum of graph parameters.
# symmetry factor Doesn't exist as a graph param

# import the translation and call the relevant function
set max_str_idx [string first L2 [pwd]]
# figure out script location
set path_to_script [file dirname [file normalize [info script]]]
# confit_translation is in the same dir as this script
source ${path_to_script}/config_translation.tcl
set param_map [get_param_map $libElement]

# -------------------------
# --- Print to Terminal ---
# -------------------------
set outFile [open $outConfig w]
puts $outFile "\{"
puts $outFile "  \"spec\": \"${libElement}.json\","
puts $outFile "  \"outdir\": \"./${libElement}_generated_graph\","
puts $outFile "  \"parameters\":\{"

set firLen -1
set ptSize -1
set isCmplxCoeff 0
set tdmChannels 1
for {set i 3} { $i < [llength $argv] } { incr i 2 } {
    set make_param [lindex $argv $i]
    set make_param_value [lindex $argv [expr ($i+1)]]
    # treat param_map as a dict of key values
    # key is make param and value is graph param
    set graph_param [dict get $param_map $make_param]

    # if we don't explictly silence a given parameter.
    if { $graph_param != ""} {

        if { $make_param eq "FIR_LEN" } {
            set firLen $make_param_value
        }
        if { $make_param eq "TDM_CHANNELS" } {
            set tdmChannels $make_param_value
        }
        if { $make_param eq "POINT_SIZE" } {
            set ptSize $make_param_value
        }
        if { $make_param eq "COEFF_TYPE" } {
            if { [string first "c" $make_param_value] != -1 } {
                puts "Coeffs are complex"
                set isCmplxCoeff 1
            }
        }
        if { $make_param eq "COARSE_BITS" } {
            set coarseBits $make_param_value
        }


        set isComma ","
        # Don't put a comma on the last parameter if we still have dummy constructor coeffs/weights to generate
        if { [expr ($i+1)] == [expr [llength $argv]-1] } {
            # FIRs and FFT Window both need a comma
            if { $libElement eq "fft_ifft_dit_1ch" || $libElement eq "matrix_mult" || $libElement eq "dds_mixer" || $libElement eq "dds_mixer_lut" || $libElement eq "mixed_radix_fft" || $libElement eq "dft" || $libElement eq "matrix_vector_mul" || $libElement eq "sample_delay" || $libElement eq "widget_real2complex"  || $libElement eq "widget_api_cast" || $libElement eq "hadamard" || $libElement eq "kronecker" || $libElement eq "outer_tensor" || $libElement eq "conv_corr" || $libElement eq "bitonic_sort" || $libElement eq "euclidean_distance" } {
                set isComma ""
            } elseif { $libElement eq "fir_sr_asym" || $libElement eq "fir_sr_sym" || $libElement eq "fir_interpolate_hb" || $libElement eq "fir_decimate_hb" || $libElement eq "fir_interpolate_asym" || $libElement eq "fir_decimate_asym" || $libElement eq "fir_decimate_sym" || $libElement eq "fir_resampler" || $libElement eq "fir_tdm" || $libElement eq "fft_window"	 || $libElement eq "func_approx"    } {
                set isComma ","
            } else {
                puts "Error: You need to add your new library element to get_common_config_json.tcl"
                exit 1
            }
        }

        puts "$make_param becomes $graph_param and has value $make_param_value"
        if { [string is double $make_param_value] } {
            puts $outFile "    \"$graph_param\": $make_param_value${isComma}"
        } else {
            # Enclose in quotes as it's likely a string like data type;
            puts $outFile "    \"$graph_param\": \"$make_param_value\"${isComma}"
        }

    }

}

set string2 "_sym"
if {[string match "*$string2*" $libElement]} {
    set firLen [expr {(int($firLen + 1) / 2)}]  }

set string2 "_hb"
if {[string match "*$string2*" $libElement]} {
    set firLen [expr {(int($firLen + 1) / 4) + 1}]  }

set coeffs {}
if { $firLen != -1 } {
    # double taps generated if complex
    set cplx [expr $isCmplxCoeff + 1]
    for {set i 0} {$i < [expr $cplx * $firLen * $tdmChannels]} {incr i} {
        lappend coeffs $i
    }
    set coeffVectorStr [join $coeffs ","]
    puts $outFile "    \"coeff\": \[$coeffVectorStr\]"
}

set weights {}
if { $libElement eq "fft_window" } {
    # double taps generated if complex
    for {set i 0} {$i < $ptSize} {incr i} {
        lappend weights $i
    }
    set weightsVectorStr [join $weights ","]
    puts $outFile "    \"weights\": \[$weightsVectorStr\]"
}

set approx_lookups {}
if { $libElement eq "func_approx" } {
    set numLutSections [expr 1  << $coarseBits]
    # double taps generated if complex
    for {set i 0} {$i < [expr $numLutSections]} {incr i} {
        # two values per lutSection - slope/offset
        lappend approx_lookups $i
        lappend approx_lookups $i
    }
    set lutVectorStr [join $approx_lookups ","]
    puts $outFile "    \"lookup_values\": \[$lutVectorStr\]"
}


puts $outFile "  }"
puts $outFile "}"

puts "Created $outConfig"

close $outFile
