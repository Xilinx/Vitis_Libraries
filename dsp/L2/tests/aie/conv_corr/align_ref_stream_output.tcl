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

if { [lsearch $argv "-h"] != -1 || $argc < 7 } {
    puts "Usage: tclsh align_ref_stream_output.tcl <ref_file> <uut_file> <phases> <casc_len> <ref_f_len> <g_len> <data_g>"
    exit 0
}

set inp_ref_file   [lindex $argv 0]
set inp_uut_file   [lindex $argv 1]
set tp_phases      [lindex $argv 2]
set tp_casc_len    [lindex $argv 3]
set tp_window_size [lindex $argv 4]   ;# REF_F_LEN = F_LEN * NITER_UUT
set tp_g_len       [lindex $argv 5]   ;# G_LEN (= tp_Sig_len in align_stream_output.tcl)
set tt_data_g      [lindex $argv 6]   ;# DATA_G type string

puts "Aligning REF/UUT stream outputs: phases=$tp_phases casc_len=$tp_casc_len g_len=$tp_g_len"

# ---------------------------------------------------------------------------
# Helper procs (same as align_stream_output.tcl)
# ---------------------------------------------------------------------------
proc getNumOfPoints {tt_data} {
    if {$tt_data eq "cint16"} {
        return 2
    } elseif {$tt_data eq "int16"} {
        return 4
    }
    # For other types (int32, cint32, float, etc.) streams_per_core defaults to 1.
    return 0
}

proc ceil_div {x y} {
    return [expr {($x + $y - 1) / $y}]
}

# ---------------------------------------------------------------------------
# Compute delay_offset  (= kDelayOffset = kStartSampleREF in C++)
#   delay_offset = MAC4ROTDELAY * TP_PHASES * (phase_incr - 1)
#   where MAC4ROTDELAY = 3  and
#         phase_incr    = TP_PHASES / streams_per_core
# ---------------------------------------------------------------------------
set Lanes  4
set Points [getNumOfPoints $tt_data_g]

# streams_per_core: 1 unless G_LEN fits within half the per-core vector width
set offset [expr {$tp_phases * $Lanes * $Points}]
if { $tp_g_len > [expr {$offset / 2}] } {
    set streams_per_core 1
} else {
    set streams_per_core 2
}

set phase_incr       [expr {$tp_phases / $streams_per_core}]
set phaseincr_offset [expr {$phase_incr - 1}]
set delay_offset     [expr {3 * $tp_phases * $phaseincr_offset}]

# ---------------------------------------------------------------------------
# Compute kCascDelayLast – mirrors the unsigned 32-bit wrap arithmetic in
# conv_corr_stream_align::kCascDelayLast (conv_corr.hpp).
#
# All intermediate values stay within 64-bit Tcl integers; uint32{} masks
# each result to 32 bits so that wrap-around matches the C++ unsigned type.
# int32{} then reinterprets the 32-bit pattern as a signed value.
#
# Guard: kLastKernelPos == 0 (CASC_LEN == 1) → no CASC_IN kernel → delay = 0.
# ---------------------------------------------------------------------------
proc uint32 {x} { return [expr {$x & 0xFFFFFFFF}] }
proc int32  {x} {
    set x [uint32 $x]
    return [expr {($x >= 0x80000000) ? ($x - 0x100000000) : $x}]
}

set kMuls_val  [expr {$Lanes * $Points}]   ;# = 8 for cint16
set kLast      [expr {$tp_casc_len - 1}]   ;# kLastKernelPos

set kCascDelayLast_signed 0
if {$kLast > 0} {
    set kMaxMuls_val  [expr {$kMuls_val * $tp_casc_len * $tp_phases}]
    set kMacsPerCore  [ceil_div $tp_g_len $kMaxMuls_val]

    # kCascDelayComp  (unsigned 32-bit)
    set inner_u32     [uint32 [expr {3 - $kMacsPerCore * $Points * 4}]]
    set shift_a       [expr {$kLast >> $phaseincr_offset}]
    set kCascDelayComp [uint32 [expr {$inner_u32 * $shift_a}]]

    # kPhaseDelayComp  (unsigned 32-bit)
    set term1_a       [uint32 [expr {$shift_a - 1}]]
    set term1         [uint32 [expr {3 * $term1_a * $phaseincr_offset}]]
    set term2         [expr {3 * ($kLast & $phaseincr_offset)}]
    set kPhaseDelayComp [uint32 [expr {$term1 + $term2}]]

    set kCascDelayLast_signed [int32 [uint32 [expr {$kCascDelayComp + $kPhaseDelayComp}]]]
}

# kStartSampleUUT – mirrors conv_corr_stream_align::kStartSampleUUT
set kStartSample [expr {($tp_g_len + $delay_offset) / $tp_phases + $kCascDelayLast_signed}]

# PLIO alignment correction for odd per-phase output counts
set per_phase_total [expr {$tp_window_size / $tp_phases}]
set per_phase_valid [expr {$per_phase_total - $kStartSample}]
set plio_loss       [expr {$per_phase_valid % 2}]

# UUT tail clip computation
set tail_trim_base    [expr {$tp_phases * $kStartSample - $delay_offset + $tp_phases * $plio_loss}]
set uut_tail_clip_raw [expr {$tp_g_len - $tail_trim_base}]
set uut_tail_clip     [expr {$uut_tail_clip_raw > 0 ? $uut_tail_clip_raw : 0}]

# X86sim correction: when plio_loss > 0, x86sim produces 1 extra sample compared to REF
# Add 1 to uut_tail_clip to compensate (this extra clip only affects x86sim, not aiesim)
if {$plio_loss > 0} {
    set uut_tail_clip [expr {$uut_tail_clip + 1}]
}

set tail_trim         [expr {$tail_trim_base + $uut_tail_clip}]

puts "  delay_offset=$delay_offset kStart=$kStartSample plio_loss=$plio_loss tail_trim=$tail_trim uut_clip=$uut_tail_clip"

# ---------------------------------------------------------------------------
# Trim the REF output file in-place
#   startIndex = samplecount_REF * 2      (×2: complex word pairs in file)
#   endIndex   = (REF_F_LEN*2 - 1) - tail_trim*2
# ---------------------------------------------------------------------------
proc align_outdata {input_filename len_window len_of_sig sample_count delay_offset} {
    set fd [open $input_filename r]
    set indata [read $fd]
    close $fd

    # Flatten all whitespace-separated tokens
    set lines [split [string trim $indata] "\n"]
    set all_elements {}
    foreach line $lines {
        set line_words [regexp -all -inline {\S+} $line]
        lappend all_elements {*}$line_words
    }

    # Compute extraction window (×2: complex word pairs in file)
    set startIndex [expr {$sample_count * 2}]
    set endIndex   [expr {($len_window * 2 - 1) - ($len_of_sig + $delay_offset - $sample_count) * 2}]

    # Extract and reformat: 4 numbers per line with trailing space
    set extracted [lrange $all_elements $startIndex $endIndex]
    set joined [join $extracted " "]
    append joined " "
    set newindata [regsub -all {\s+} $joined " "]
    set newindata [regsub -all {(\S+\s+\S+\s+\S+\s+\S+) } $newindata "\\1 \n"]

    set fd [open $input_filename w]
    puts -nonewline $fd $newindata
    close $fd
}

# Trim REF output file
align_outdata $inp_ref_file $tp_window_size $tail_trim $delay_offset $delay_offset

# Trim UUT output file tail if needed
if {$uut_tail_clip > 0} {
    set uut_total [expr {$tp_window_size - $tp_phases * $kStartSample - $tp_phases * $plio_loss}]
    align_outdata $inp_uut_file $uut_total $uut_tail_clip 0 0
}

puts "Alignment complete."
