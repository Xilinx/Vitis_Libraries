#!/tools/xgs/perl/5.8.8/bin/perl -w
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

use Cwd;
use Cwd 'chdir';
use Getopt::Long;
my $usage = "This script will split a single input file: \"./data/input.txt\" into multiple.\n
Options:
    -h - display this help
    -t - number of output files
    -g - sample granularity

";
my $debug = 0;
my $help = 0;
my $numFiles = 0;
my $numSamplesPerFile = 1;
my $i = 0;

if ($debug == 1) {print "Entering parse_args...";}
GetOptions('h'   => \$help,
           't=i' => \$numFiles,
           'g=i' => \$numSamplesPerFile
  )    or die "$usage";
die "$usage\n" if $help;

my @outfiles;
my $dir = "./data";
print "starting split with ${numFiles} outputs\n";
opendir(D,$dir) or die "could not open pthread dir\n";
for ($i = 0; $i < $numFiles; $i++) {
  open($outfiles[$i], ">${dir}/input$i.txt");
}
my $idx = 0;
my $sampleIdx = 0;
open(ORIG, "${dir}/input.txt");
my $text;
while(<ORIG>) {
  $text = $_;
  print {$outfiles[$idx]} $text;
#  print "printing $text to $idx\n";
  $sampleIdx = $sampleIdx + 1;
  if ($sampleIdx == $numSamplesPerFile) {
    $sampleIdx = 0;
    $idx = $idx+1;
  }
  if ($idx == $numFiles) {
    $idx = 0;
  }
}
for ($i = 0; $i < $numFiles; $i++) {
  close($outfiles[$i]);
}
close(ORIG);
closedir(D);
print "finished splitting input file into files\n";
exit 0;
#successful end to program
###################################################################
