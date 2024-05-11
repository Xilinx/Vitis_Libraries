#!/usr/bin/perl
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
use strict;
use warnings;
#use File::Compare;

# open each file and assign to an array
open my $ref_json, '<', $ARGV[0] or die "Can't open file: $!";
my @lines1 = <$ref_json>;
close $ref_json;

open my $out_json, '<', $ARGV[1] or die "Can't open file: $!";
my @lines2 = <$out_json>;
close $out_json;

#remove last 3 lines
splice(@lines1, -3);
splice(@lines2, -3);


# checking if the files are same
if (join("",@lines1) eq join("",@lines2))
{
    print "Files are identical. \n";
}
 
# checking if the files are different
else
{
    print "ERROR: Files are not identical. \n";
}

exit;
