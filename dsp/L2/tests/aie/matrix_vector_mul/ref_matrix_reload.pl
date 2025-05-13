#!/usr/bin/perl
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
use strict;
use warnings;

# Script specific to the matrix_vector_mul to test RTP on UUT without using RTP on reference model
# UUT -> update RTP with matrix 1, run NITER/2, update RTP with matrix 2
# Ref -> matrix 1, matrix1, matrix1 ..(NITER/2).. matrix1, matrix2, matrix2, matrix2
#
# gen_input should be configured to produce 2 iterations of data when USE_MATRIX_RELOAD=1. First half of generated input is written to a new file niter/2 times, then the second half is written niter/2 times.

# Check for correct number of arguments
if (@ARGV != 3) {
    die "Usage: $0 <use_rtp> <filename> <niter>\n";
}

my ($use_rtp, $filename, $niter) = @ARGV;

# Check if niter is a positive integer
if ($niter !~ /^\d+$/ || $niter <= 0) {
    die "niter must be a positive integer.\n";
}

if ($use_rtp == 0) {
    exit;
}
# Read the file
open my $fh, '<', $filename or die "Cannot open file '$filename': $!\n";
my @lines = <$fh>;
close $fh;

# Calculate the number of lines
my $numLines = scalar @lines;

# Split the lines into two halves
my $half = $numLines / 2;
my @first_half = @lines[0 .. $half - 1];
my @second_half = @lines[$half .. $numLines - 1];

# Open the file for writing
open my $out_fh, '>', $filename or die "Cannot open file '$filename' for writing: $!\n";

# Write each half niter/2 times
for (1 .. $niter / 2) {
    print $out_fh @first_half;
}
for (1 .. $niter / 2) {
    print $out_fh @second_half;
}

close $out_fh;

print "File '$filename' has been updated successfully.\n";
