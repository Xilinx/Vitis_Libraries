#!/usr/bin/perl -w
#
# Copyright 2022 Xilinx, Inc.
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
use Cwd;
use Cwd 'chdir';
use Getopt::Long;
use File::Basename;

use Term::ReadLine;



# TODO: accept STDIN as well as inFile.
# TODO: Granularity
# TODO: int16
# TODO: Lots of error checking


my $usage = "
This script will split an input text file into an SSR number of input text files or interleave (zip) multiple files into one file.
use it thus:
ssr_split_zip.pl -f data/input.txt --ssr 2  --split --dual 1 -t cint16
    The above will output data/input_0.txt data/input_1.txt, where input_0.txt has samples 0, 2, 4, 6, 8.. and input_1.txt has samples 1, 3, 5...
options:
    -f|--file=s                                       => filepath containing data to be split or resultant filepath containing interleaved data.
    [-s|--ssr=i]                                      => ssr number - number of files to split over
    [--split | --zip]                                 => Controls wether to split an input file into SSR sub-files or to combine (zip) SSR sub-files into one output file
    [-d | --dual]                                     => If using dual input/output streams, SSR phases are split into io1 and io2 with a granularity determined by the data type. 
    [-t|--type=i]                                     => Data type for the file - note int16 has two samples per line.
     [-h|--help]                                      => Optional. prints this usage
    [-v|--verbose]                                    => Optional. additional logging
";

my $file = "";
my $ssr = 1;
my $split = 0;
my $zip = 0;
my $type = "cint16";
my $dual = 0;
my $help = 0;
my $verbose = 0;


GetOptions (
    "f|file=s" => \$file,
    "s|ssr=i"  => \$ssr,
    "split" => \$split,
    "zip" => \$zip,
    "t|type=s" => \$type,
    "d|dual=i" => \$dual,
    "h|help" => \$help,
    "v|verbose" => \$verbose)
or die("Error in command line arguments\n");

if ( $help ) {
    die "$usage";
}

# Handle mandatory arguments
if ( $file eq "" ) {
    die "ERROR: need a filename input. -h for usage"
}


if ( ($split and $zip) or ((not $split) and (not $zip)) ) {
    die "ERROR: need only split or zip. -h for usage"
}

#"i|g|interleaveGranularity|granularity=i" => \$gran,
# int16 has two samples per line, but we hanlde this when we initially read file; so that it IS 1 line per sample.
my $linesPerSample = 1;
if ($type eq "cint32" or $type eq "cfloat") {
  # A single complex sample is split over two lines
  $linesPerSample = 2;
}
my $samplesPerLine = 1;
if ($type eq "int16" ) {
  # A single line has 32 bits of information, so two real int16 samples.
  $samplesPerLine = 2;
}
my $dual_gran;
my $data_type_size_bytes;
if ($type eq "cint32")  {
	$data_type_size_bytes = 8;
} elsif ($type eq "cfloat") {
	$data_type_size_bytes = 8;
} elsif ($type eq "cint16") {
	$data_type_size_bytes = 4;
} elsif ($type eq "int32") {
	$data_type_size_bytes = 4;
} elsif ($type eq "float") {
	$data_type_size_bytes = 4;
} else {
	$data_type_size_bytes = 2;
}

use integer;
#128b granularity for each data type
$dual_gran = ((128/8) / $data_type_size_bytes );

print("Splitting/Zipping to SSR files then each file into dual streams of granuarity ${dual_gran}.\n") if ($dual == 1);

# get component parts of input/output filenames
(my $fileName, my $fileDir, my $fileExt) = fileparse($file, '\..*');

my @ssrRange = (0...($ssr - 1));
my @dualRange = (0...($dual));
print @dualRange;
my @subFiles;
my @subFilesH;
# Used if int16 two samples per line
my @subFilesFinal;
my @subFilesFinalH;
if ($split) {
    print "reading $file\n";
    open(fileH, "<", $file)
        or die "cannot open $file : $!";
    my $outFileMod = "";

    if  ($samplesPerLine > 1) { 
      open(fileH_mod, ">", "${file}_int16mod")
          or die "cannot open ${file}_int16Mod : $!";

      # split two samples per line into two lines
      while (<fileH>) {
          s/\s+(-?[0-9]+)\s?/\n$1/g ;
          print fileH_mod $_;
      }

      close(fileH)
          or die "couldn't close $file";
      close(fileH_mod)
          or die "couldn't close ${file}_int16mod";
      # reset fileH to point to the file we just created
      open(fileH, "<", "${file}_int16mod")
          or die "cannot open ${file}_int16Mod : $!";
      $outFileMod = "_beforeInt16Mod";
    }

    # Open a bunch of files in an array of file handles with a specific filename
    for my $ssrIdx (@ssrRange){
      for my $dualIdx (@dualRange){
          my $fileIdx = $ssrIdx*($dual + 1) + $dualIdx;
          $subFiles[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${dualIdx}${fileExt}${outFileMod}";
          print "Writing to $subFiles[$fileIdx]\n";
          open($subFilesH[$fileIdx], ">", $subFiles[$fileIdx])
              or die "cannot open $subFiles[$fileIdx] : $!";
      }
    }
      
    my $lineNum = 0;
    my $ssrIndex = 0;
    my $dualIdx = 0;
    my $linesPerDualStream = $linesPerSample * $ssr * $dual_gran;
    while(my $line = <fileH>){

      my $fileIdx = $ssrIndex*($dual + 1) + $dualIdx;
      print {$subFilesH[$fileIdx]} $line;
      if ($lineNum % $linesPerSample == ($linesPerSample-1)) {
        $ssrIndex = ($ssrIndex+1) % $ssr;
      }
      if ($lineNum % ($linesPerDualStream) == (($linesPerDualStream)-1)) {
        $dualIdx = ($dualIdx+1) % ($dual+1);
      }
      $lineNum = $lineNum+1;
    }
    
    if  ($samplesPerLine > 1) { 

      # Simply open all the output files and force two samples per line
      for my $ssrIdx (@ssrRange){
        for my $dualIdx (@dualRange){
          my $fileIdx = $ssrIdx*($dual + 1) + $dualIdx;
          #close for Write
          close($subFilesH[$fileIdx])
              or die "cannot close $subFiles[$fileIdx] : $!";
          # Open out files for read
          open($subFilesH[$fileIdx], "<", $subFiles[$fileIdx])
              or die "cannot open $subFiles[$fileIdx] : $!";

          print "Reading from $subFiles[$fileIdx]\n";
          # intendedfinal output file without outFileMod
          $subFilesFinal[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${dualIdx}${fileExt}";
          print "Writing to $subFilesFinal[$fileIdx]\n";

          open($subFilesFinalH[$fileIdx], ">", $subFilesFinal[$fileIdx])
              or die "cannot open $subFilesFinal[$fileIdx] : $!";

          my $ssrHandle = $subFilesH[$fileIdx];
          my $ssrHandleFinal = $subFilesFinalH[$fileIdx];
          my $lineCount = 0;
          while(my $line = <$ssrHandle>){
            # Every even line we remove the newline character.
            if ($lineCount % $samplesPerLine == 0) {
              $line =~ s/\n/ /g ;
            }
            print $ssrHandleFinal $line;
            $lineCount++;
          }
        }
      }
    }

} elsif ($zip) {
    print "Will write to $file\n";
    open(my $fileH, ">", $file)
        or die "cannot open $file : $!";

    my @ssrLines;
    for my $ssrIdx (@ssrRange){
      for my $dualIdx (@dualRange){
        # create file names
        my $fileIdx = $ssrIdx*($dual + 1) + $dualIdx;
        $subFiles[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${dualIdx}${fileExt}";
        print "Reading from $subFiles[$fileIdx]\n";
        # open for read
        open($subFilesH[$fileIdx], "<", $subFiles[$fileIdx])
            or die "cannot open $subFiles[$fileIdx] : $!";

        # read all the lines from file
        my $ssrHandle = $subFilesH[$fileIdx];
        my @lines = <$ssrHandle>;
        # Make sure we get rid of timestamp with the same grep in makefile.
        @lines = grep !/[XT]/, @lines;
        #print @lines;
        #re-index the lines according to SSR
        my $numLines = @lines;
        print "There are $numLines lines in file\n";
        if (($type eq "int16")) { 
          $numLines = $numLines*2; # two samples per line
        }
        for my $lineInFileIdx (0...($numLines - 1)) {
          my $lineIdx = ($lineInFileIdx / $samplesPerLine);
          my $innnerSampleIdx = ($lineInFileIdx % $linesPerSample);
          my $sampleIdx = ($lineInFileIdx/$linesPerSample);
          my $linesBetweenSsrSamples = ($linesPerSample * $ssr);

          my $innerDualStreamChunk = (($sampleIdx % $dual_gran) * $linesBetweenSsrSamples);
          my $outerDualStreamChunk = (($sampleIdx / $dual_gran) * $linesBetweenSsrSamples * ($dual_gran * ($dual+1)) );

          my $offsetForDualStreamIdx = ($dualIdx*$dual_gran*$linesBetweenSsrSamples);
          my $offsetForSsrIdx = ($ssrIdx*$linesPerSample);

          my $resIdx = ( $innnerSampleIdx + $innerDualStreamChunk +  $outerDualStreamChunk  + $offsetForDualStreamIdx + $offsetForSsrIdx);
          #print "$resIdx for file${ssrIdx}_${dualIdx} line $lineIdx\n s$sampleIdx; $innnerSampleIdx + $innerDualStreamChunk +  $outerDualStreamChunk  + $offsetForDualStreamIdx + $offsetForSsrIdx \n";
          my $line = $lines[$lineIdx];

          if ($samplesPerLine > 1) { 
            # a capturing group for each sample on the line (wouldn't work for floats)
            $line =~ /^((?:-)?\d+ )((?:-)?\d+ )?/;
            my ($firstSample,$secondSample) = ($1,$2);
            if ($lineInFileIdx % $samplesPerLine == 0) {
              # odd index resIdx means we take a new line
              if ($resIdx % $samplesPerLine == ($samplesPerLine-1)) { 
                $ssrLines[$resIdx] = "$firstSample\n";
              } else { 
                $ssrLines[$resIdx] = "$firstSample"; 
              }
            } else { 
              # odd index resIdx means we take a new line
              if ($resIdx % $samplesPerLine == ($samplesPerLine-1)) { 
                $ssrLines[$resIdx] = "$secondSample\n";
              } else { 
                $ssrLines[$resIdx] = "$secondSample"; 
              }
            }
          } else { 
            $ssrLines[$resIdx] = $line;
          }
        }
      }
    }
    # newly re-arranged lines to result file
    print $fileH @ssrLines;
    close($fileH);
}
