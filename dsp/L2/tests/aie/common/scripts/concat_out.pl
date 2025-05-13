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
my $usage = "This script will combine multiple input files into one output file.\n";
my $debug = 0;
my $help = 0;
my $numFiles = 0;
my $i = 0;
my $k = 0;
my $ssr_form = 0; #0 = chunks, 1 = samplewise
my $parallel_factor = 0;
my $point_size = 0;
my $splice_len;
my $fileprefix;
my $filepostfix = "fft_ifft_dit_1ch_graph_cint16_cint16_32_1_0_1_0_32_1_1_SRC_x86sim_5";
my $use_postfix = 1;
my $line = "";
my @indata;
my $sampleIndex = 0;

if ($debug == 1) {print "Entering parse_args...";}
GetOptions('h'   => \$help,
           't=i' => \$numFiles,
           'p=i' => \$parallel_factor,
           's=i' => \$point_size,
           'q=s' => \$fileprefix,
           'v=i' => \$use_postfix,
           'w=s' => \$filepostfix,
           'r=i' => \$ssr_form
  )    or die "$usage";
die "$usage\n" if $help;

my @infiles;
my $dir = "./data";
$splice_len = $point_size/(2**($parallel_factor+1));
print "starting concat with ${numFiles} input files\n";
opendir(D,$dir) or die "could not open pthread dir\n";
my $linenum = 0;
for ($i = 0; $i < $numFiles; $i++) {
  if ($use_postfix == 1) {
  #  print "trying to open ${dir}/${fileprefix}${i}_${filepostfix}.txt\n";
    open(MYFH, "<${dir}/${fileprefix}${i}_${filepostfix}.txt") or print "Error: concat.out.pl cannot access file ${dir}/${fileprefix}${i}_${filepostfix}.txt\n";
  } else {

  #  print "trying to open ${dir}/${fileprefix}${i}.txt\n";
    open(MYFH, "<${dir}/${fileprefix}${i}.txt") or print "Error: concat.out.pl cannot access file ${dir}/${fileprefix}${i}_${filepostfix}.txt\n";
  }
  $linenum = 0;
  while(<MYFH>) {
    $indata[$i][$linenum] = $_;
    $linenum++;
  }
  close(MYFH);
}
my $idx = 0;
if ($use_postfix == 1) {
  open(OUTFILE, ">${dir}/${fileprefix}_${filepostfix}.txt");
} else {
  open(OUTFILE, ">${dir}/${fileprefix}.txt");
}
if ($ssr_form == 0) {
  for (my $splices = 0; $splices < $linenum/$splice_len; $splices++) {
    for ($i = 0; $i < $numFiles; $i++) {
      for($k = 0; $k < $splice_len; $k++) {
        $line = $indata[$i][$k+$splices*$splice_len];
        print OUTFILE $line;
      }
    }
  }
} else {
  for (my $splices = 0; $splices < $linenum/$splice_len; $splices++) {
    for($k = 0; $k < $splice_len; $k++) {
      for ($i = 0; $i < $numFiles; $i++) {
        $line = $indata[$i][$k+$splices*$splice_len];
        print OUTFILE $line;
      }
    }
  }
}
close(OUTFILE);
closedir(D);
print "finished merging output files\n";
exit 0;
#successful end to program
###################################################################
