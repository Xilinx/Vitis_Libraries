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
    [-k|--headerMode=i]                               => Header Embedded in Data stream. Supported modes: 0 - no header, 1 - fixed length, 2 - variable length, length embedded in stream.
    [-w|--windowVsize=i]                              => Number of samples per window
     [-h|--help]                                      => Optional. prints this usage
    [-v|--verbose]                                    => Optional. additional logging
";

my $file = "";
my $ssr = 1;
my $split = 0;
my $zip = 0;
my $type = "cint16";
my $dual = 0;
my $headerMode = 0;
my $windowVsize = 0;
my $coeffType = 0;
my $firLen = 0;
my $help = 0;
my $verbose = 0;


GetOptions (
    "f|file=s" => \$file,
    "s|ssr=i"  => \$ssr,
    "split" => \$split,
    "zip" => \$zip,
    "t|type=s" => \$type,
    "d|dual=i" => \$dual,
    "k|headerMode=i" => \$headerMode,
    "w|windowVsize=i" => \$windowVsize,
    "c|coeffType=s" => \$coeffType,
    "fl|firLen=i" => \$firLen,
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
my $coeffLinesPerSample = 1;
if ($coeffType eq "cint32" or $coeffType eq "cfloat") {
  # A single complex sample is split over two lines
  $coeffLinesPerSample = 2;
}
my $samplesPerLine = 1;
if ($type eq "int16" ) {
  # A single line has 32 bits of information, so two real int16 samples.
  $samplesPerLine = 2;
}
my $partsPerLine = 1;
if ($type eq "cint16" or $type eq "int16") {
  # 2 parts per single line
  $partsPerLine = 2;
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
my $sizeOfCoeff;
if ($coeffType eq "cint32" || $coeffType eq "cfloat") {
    $sizeOfCoeff = 8;
} elsif ($coeffType eq "cint16" || $coeffType eq "int32" || $coeffType eq "float" ) {
    $sizeOfCoeff =4;
} elsif ($coeffType eq "int16") {
    $sizeOfCoeff = 2;
} else {
    $sizeOfCoeff = 2;
}

my $complex = 0;
if ($type eq "cint32" || $type eq "cint16" || $type eq "cfloat") {
  $complex = 1;
}

my $headerVsize = 0;
if ($headerMode != 0 ) {
  if ($type eq "int16" ) {
    $headerVsize = 16;
  } elsif ($type eq "cint16" || $type eq "int32" || $type eq "float") {
    $headerVsize = 8;
  } else {
    $headerVsize = 4;
  }
} else {
  $headerVsize = 0;
}
my $dualFactor = $dual+1; #i.e. when dual=1, dualFactor=2 otherwise =1

use integer;
#128b granularity for each data type
$dual_gran = ((128/8) / $data_type_size_bytes );

print("Splitting/Zipping to SSR files then each file into dual streams of granuarity ${dual_gran}.\n") if ($dual == 1);

# get component parts of input/output filenames
(my $fileName, my $fileDir, my $fileExt) = fileparse($file, '\..*');

my @ssrRange = (0...($ssr - 1));
my @dualRange = (0...($dual));
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

    # Rearrange files with multiple samples per line to 1 sample per line and twice the lines.
    # This is set up for data types of 16 bits and less.
    # This is required since SSR splits input stream on a sample-by-sample basis.
    if  ($samplesPerLine > 1) {

      # Parse input files
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

      # Prep output file name suffix
      $outFileMod = "_beforeInt16Mod";
    }

    # Open a bunch of files in an array of file handles with a specific filename
    for my $ssrIdx (@ssrRange){
      for my $dualIdx (@dualRange){
          my $fileIdx = $ssrIdx*$dualFactor + $dualIdx;
          $subFiles[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${dualIdx}${fileExt}${outFileMod}";
          print "Writing to $subFiles[$fileIdx]\n";
          open($subFilesH[$fileIdx], ">", $subFiles[$fileIdx])
              or die "cannot open $subFiles[$fileIdx] : $!";
      }
    }

    my $lineNum = 0;
    my $lineNumHeader = 0;
    my $ssrIndex = 0;
    my $dualIdx = 0;
    my $linesPerDualStream = $linesPerSample * $dual_gran;
    my $linesPerSSRDualStream = $linesPerDualStream * $ssr;
    if ($headerMode == 1) {


      my $this_header_lines = $headerVsize*$linesPerSample;
      my $this_frame_lines = $windowVsize*$linesPerSample;
      while(my $line = <fileH>){
        if ($this_header_lines > 0) {
          for my $sI (@ssrRange){
            for my $dI (@dualRange){
              my $fI = $sI*$dualFactor+ $dI;
              print {$subFilesH[$fI]} $line;
            }
          }
          $this_header_lines = $this_header_lines -1;
          $this_frame_lines = $windowVsize*$linesPerSample;
        } else {
          if ($this_frame_lines > 0) {
            my $fileIdx = $ssrIndex*$dualFactor + $dualIdx;
            print {$subFilesH[$fileIdx]} $line;
            if ($lineNum % $linesPerSample == ($linesPerSample-1)) {
              $ssrIndex = ($ssrIndex+1) % $ssr;
            }
            if ($lineNum % ($linesPerSSRDualStream) == (($linesPerSSRDualStream)-1)) {
              $dualIdx = ($dualIdx+1) % $dualFactor;
            }
            $lineNum = $lineNum+1;
            $this_frame_lines = $this_frame_lines -1;
            if ($this_frame_lines == 0) {
              $this_header_lines = $headerVsize*$linesPerSample;
            }
          }
        }
      }
    } elsif ($headerMode == 2) {
      # extract header length from data stream and broadcast header to all ssr files.
      # Then read data frame and split according to config.

      my $this_header_init = 0;
      my $headerCoeffArraySize = 0;
      my $this_header_lines = $headerVsize*$linesPerSample;
      my $this_frame_lines = $windowVsize*$linesPerSample;
      while(my $line = <fileH>){
        # Initialize once per frame
        if ($this_header_init == 0) {
          $this_header_init = 1;
          # Extract coefficient length from file. Either only value, when 1 sample or first out of 2, for cint16/int16.
          my @this_size = split ' ', $line;
          $headerCoeffArraySize = $this_size[0];
          $this_header_lines = ($headerVsize + $headerCoeffArraySize * $sizeOfCoeff / $data_type_size_bytes) * $linesPerSample;
          $lineNum = 0;
          $lineNumHeader = 0;
        }


        if ($this_header_lines > 0) {
          # Read out Header and send full header to all SSR paths.

          # Copy the header across in full to both streams when dual streams used.
          # for my $sI (@ssrRange){
          #   for my $dI (@dualRange){
          #     my $fI = $sI*$dualFactor+ $dI;
          #     print {$subFilesH[$fI]} $line;
          #   }
          # }

          # Split the header across both streams when dual streams used.
          for my $sI (@ssrRange){
              my $fI = $sI*$dualFactor+ $dualIdx;
              print {$subFilesH[$fI]} $line;
          }
          if ($lineNumHeader % ($linesPerDualStream) == (($linesPerDualStream)-1)) {
            $dualIdx = ($dualIdx+1) % $dualFactor;
          }
          $lineNumHeader = $lineNumHeader+1;
          $this_header_lines = $this_header_lines -1;
          $this_frame_lines = $windowVsize*$linesPerSample;
        } else {
          # Read out the frame data samples and split samples across ssr paths, further splitting each ssr path between dual streams when used.
          if ($this_frame_lines > 0) {

            my $fileIdx = $ssrIndex*$dualFactor + $dualIdx;
            print {$subFilesH[$fileIdx]} $line;
            if ($lineNum % $linesPerSample == ($linesPerSample-1)) {
              $ssrIndex = ($ssrIndex+1) % $ssr;
            }
            if ($lineNum % ($linesPerSSRDualStream) == (($linesPerSSRDualStream)-1)) {
              $dualIdx = ($dualIdx+1) % $dualFactor;
            }
            $lineNum = $lineNum+1;
            $this_frame_lines = $this_frame_lines -1;
            if ($this_frame_lines == 0) {
              $this_header_init = 0;
            }
          }
        }
      }

    } elsif ($headerMode == 299) {
      # Create Fixed header length, based on variable length input header, by extending reduced header in frames into full length header.
      # Not an official mode, but handy for the reference model operations.

      # If Header is followed by coeffs, broadcast it to all SSR files.
      # Otherwise, extend the extracted header to a full sized one by adding dummy header payload.

      my $this_header_init = 0;
      my $firLenCeiled = 0;
      my $headerCoeffArraySize = 0;
      my $this_dummy_payload = 0;
      my $this_header_lines = $headerVsize*$linesPerSample;
      my $this_frame_lines = $windowVsize*$linesPerSample;
        # print "\n this_frame_lines size:  $this_frame_lines\n";
      while(my $line = <fileH>){

        # Initialize once per frame
        if ($this_header_init == 0) {
          $this_header_init = 1;
          # Extract coefficient length from file. Either only value, when 1 sample or first out of 2, for cint16/int16.
          my @this_size = split ' ', $line;
          # Header size = 256-bit, i.e. 4 cint32/cfloat or 8 cint16/ int16 (int16 - 2 samples per line)
          my $coeffSamplesIn256Bits = 256 / 8 /  $sizeOfCoeff;
          $headerCoeffArraySize = $this_size[0];
          if ($headerCoeffArraySize == 0) {
            $firLenCeiled =  $firLen + ($coeffSamplesIn256Bits - $firLen % $coeffSamplesIn256Bits)% $coeffSamplesIn256Bits;
            $this_dummy_payload = $firLenCeiled * $sizeOfCoeff * $linesPerSample / $data_type_size_bytes;
          } else {
            $this_dummy_payload = 0

          }
          $this_header_lines = ($headerVsize + $this_size[0] * $sizeOfCoeff / $data_type_size_bytes) * $linesPerSample;
          $lineNum = 0;
          $lineNumHeader = 0;

        }

        if ($this_header_lines > 0) {
          # Read out Header and send full header to all SSR paths.

          # Copy the header across in full to both streams when dual streams used.
          # print "h";
          # for my $sI (@ssrRange){
          #   for my $dI (@dualRange){
          #     my $fI = $sI*$dualFactor+ $dI;
          #     print {$subFilesH[$fI]} $line;
          #   }
          # }

          # Split the header across both streams when dual streams used.
          for my $sI (@ssrRange){
              my $fI = $sI*$dualFactor+ $dualIdx;
              print {$subFilesH[$fI]} $line;
          }
          if ($lineNumHeader % ($linesPerDualStream) == (($linesPerDualStream)-1)) {
            $dualIdx = ($dualIdx+1) % $dualFactor;
          }
          $lineNumHeader = $lineNumHeader+1;
          $this_header_lines = $this_header_lines -1;
          $this_frame_lines = $windowVsize*$linesPerSample;
        } else {
          if ($this_dummy_payload > 0) {
            # Split the header across both streams when dual streams used.
            my @CoeffPaylodRange = (0...($this_dummy_payload - 1));

            my $dummy_line = "0\n";
            if ($partsPerLine == 2 && $samplesPerLine == 1) {
              $dummy_line = "0 0 \n";
            }

            for my $coeffPaylod (@CoeffPaylodRange){
              # print "p";
              $this_dummy_payload = $this_dummy_payload -1;

              # Copy the dummy payload across in full to both streams when dual streams used.
              # for my $sI (@ssrRange){
              #   for my $dI (@dualRange){
              #     my $fI = $sI*$dualFactor+ $dI;
              #     print {$subFilesH[$fI]} $dummy_line;
              #     #print "Writing $line to {$subFilesH[$fI]}\n";
              #   }
              # }

              # Split the dummy payload across both streams when dual streams used.
              for my $sI (@ssrRange){
                  my $fI = $sI*$dualFactor+ $dualIdx;
                  print {$subFilesH[$fI]} $dummy_line;
                  #print "Writing $line to {$subFilesH[$fI]}\n";
              }
              if ($lineNum % ($linesPerDualStream) == (($linesPerDualStream)-1)) {
                $dualIdx = ($dualIdx+1) % $dualFactor;
              }
              $lineNum = $lineNum+1;
            }
          }
          # print "d";
          # Read out the frame data samples and split samples across ssr paths, further splitting each ssr path between dual streams when used.
          if ($this_frame_lines > 0) {
            my $fileIdx = $ssrIndex*$dualFactor + $dualIdx;
            print {$subFilesH[$fileIdx]} $line;
            if ($lineNum % $linesPerSample == ($linesPerSample-1)) {
              $ssrIndex = ($ssrIndex+1) % $ssr;
            }
            if ($lineNum % ($linesPerSSRDualStream) == (($linesPerSSRDualStream)-1)) {
              $dualIdx = ($dualIdx+1) % $dualFactor;
            }
            $lineNum = $lineNum+1;
            $this_frame_lines = $this_frame_lines -1;
            if ($this_frame_lines == 0) {
              $this_header_init = 0;
            }
          }
        }
        # print "\n";
      }

    } else {

      while(my $line = <fileH>){

        my $fileIdx = $ssrIndex*$dualFactor + $dualIdx;
        print {$subFilesH[$fileIdx]} $line;
        if ($lineNum % $linesPerSample == ($linesPerSample-1)) {
          $ssrIndex = ($ssrIndex+1) % $ssr;
        }
        if ($lineNum % ($linesPerSSRDualStream) == (($linesPerSSRDualStream)-1)) {
          $dualIdx = ($dualIdx+1) % $dualFactor;
        }
        $lineNum = $lineNum+1;
      }
    }

    # Rearrange tmp output files back to multiple samples per line.
    if  ($samplesPerLine > 1) {

      # Simply open all the output files and force two samples per line
      for my $ssrIdx (@ssrRange){
        for my $dualIdx (@dualRange){
          my $fileIdx = $ssrIdx* $dualFactor + $dualIdx;
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
      close(fileH)

} elsif ($zip) {
  my $numSamples;
  my $real;
  my $imag;
  my $firstSample;
  my $secondSample= "";
  my @samples; #whole file, converted to samples
  my @headers; #concatenated headers, held as samples. (not as lines)
  my @dataSamples; #concatenated payload, held as samples (not as lines)
  my $headersIdx = 0;
  my $dataSamplesIdx = 0;
  my $sampleIdx = 0;
  my $numWindows = 0;
  my @ssrSamples; #the zipped array
  my @ssrLines; #The zipped output file/array.
  my $numDataSamples;
  print "Will write to $file\n";
  open(my $fileH, ">", $file)
    or die "cannot open $file : $!";

  for my $ssrIdx (@ssrRange){
    for my $dualIdx (@dualRange){
      # create file names
      my $fileIdx = $ssrIdx* $dualFactor + $dualIdx;
      $subFiles[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${dualIdx}${fileExt}";
      #print "Reading from $subFiles[$fileIdx]\n";
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
      #$numSamples = $numLines;
      #print "\nThere are $numLines lines in file\n";

      #convert lines to samples
      #print "\n...Converting lines to samples...\n";

      my $line;
      my @lineRange = (0...($numLines-1));
      for my $lineIdx (@lineRange) {
        if ($samplesPerLine == 2) {
          $line = $lines[$lineIdx];
          $line =~ /^((?:-)?\d+)\s+((?:-)?\d+)?/;
          ($firstSample,$secondSample) = ($1,$2);
          $samples[$lineIdx*2] = "$firstSample ";
          $samples[$lineIdx*2+1] = "$secondSample ";
        } elsif ($linesPerSample == 2) {
          $line = $lines[$lineIdx];
          $line =~ /^(.+)/;
          if ($lineIdx % 2 == 0) {
            $firstSample = $1;
          } else {
            $secondSample = $1;
            $samples[$lineIdx/2] = "$firstSample $secondSample ";
          }
        } else {
          $line = $lines[$lineIdx];
          if ($complex) {
            $line =~ /([^\s]+)\s+([^\s]+)/;
            ($real,$imag) = ($1,$2);
            $samples[$lineIdx] = "$real $imag ";
          } else {
            $line =~ /([^\s]+)/;
            $real = $1;
            $samples[$lineIdx] = "$real ";
          }
        }
      }
      #print @samples;

      #split into headers and dataSamples
      #print "\n...Separating input samples to header and dataSamples arrays...\n";
      #print "\nSamples...\n";
#      if ($ssrIdx == 0) {
#        print "\nSamples from first input...\n";
#        print @samples;
#      }

      $headersIdx = 0;
      $dataSamplesIdx = 0;
      $sampleIdx = 0;
      $numWindows=0;
      $numSamples = @samples;

      while ($sampleIdx < $numSamples) {
        $numWindows = $numWindows+1;
        for my $Idx (0...($headerVsize-1)) {
          $headers[$headersIdx] = $samples[$sampleIdx];
          $sampleIdx = $sampleIdx+1;
          $headersIdx = $headersIdx+1;
        }
        for my $idx (0...($windowVsize/($ssr*$dualFactor)-1)) {
          $dataSamples[$dataSamplesIdx] = $samples[$sampleIdx];
          $sampleIdx = $sampleIdx+1;
          $dataSamplesIdx = $dataSamplesIdx+1;
        }
      }
      $numDataSamples = $dataSamplesIdx;

      #perform zip to combined array.
      #print "\n...Zipping headers and data samples from contributing file to single output array...\n";
      #print "\n Number of windows detected = $numWindows\n";
      #print "\n Number of samples in each window = $numSamples\n";
      #print "\n dataSamplesIdx = $dataSamplesIdx\n";
      #print "Headers...\n";
      #print @headers;
      #print "\nData samples... \n";
      #print "\n\nSplit files\n\n";
      #print @dataSamples;

      #The actual zipping takes place in this loop. From one of the files, the output file is sparsely written
      my $inSampleStart = 0;
      my $inHeaderStart = 0;
      my $outStart = 0;
      for my $window (0...($numWindows-1)) {
        $outStart = $window * ($headerVsize+$windowVsize);
        for my $headerIdx (0...($headerVsize-1)) {
          $ssrSamples[$outStart+$headerIdx] = $headers[$inHeaderStart+$headerIdx];
        }
        $outStart = $outStart + $headerVsize;
        $inHeaderStart = $inHeaderStart + $headerVsize;
        #print "outStart = $outStart\n";
        #print "windowVsize/(ssr*dualFactor) = $windowVsize/($ssr*$dualFactor)\n";
        #print "numDataSamples = $numDataSamples\n";
        for my $sampleIdx (0...($windowVsize/($ssr*$dualFactor)-1)) { #$windowVsize is dangerous in FIRs.
        #for my $sampleIdx (0...($numDataSamples-1)) {
          my $dualchunk  = $sampleIdx / $dual_gran;
          my $dualoffset = $sampleIdx % $dual_gran;
          my $outIdx = $outStart +
                       $ssrIdx +
                       $ssr * $dualoffset  +
                       $ssr * $dualIdx * $dual_gran +
                       $dualchunk*$dualFactor*$dual_gran*$ssr;
          #print "Writing $dataSamples[$inSampleStart+$sampleIdx] from $inSampleStart+$sampleIdx to $outIdx for outStart=$outStart, ssrIdx=$ssrIdx, dualoffset=$dualoffset, dualIdx=$dualIdx, Dualchunk=$dualchunk, dual_gran=$dual_gran\n";
          $ssrSamples[$outIdx] =  $dataSamples[$inSampleStart+$sampleIdx];
        }
        $inSampleStart = $inSampleStart + $windowVsize/($ssr*$dualFactor);
      }
    } #dual
  } #ssr
  #print "\nZipped samples...\n";
  #print @ssrSamples;

  #convert output array to output file. What's the difference? SamplesPerLine or LinesPerSample.
  #print "\n...Converting output array to output file. This takes samples per line and lines per sample into account...\n";
  my $ssrSample;
  my $numSSRSamples = @ssrSamples;
  #print "numSSRSamples = $numSSRSamples\n";
  my @outFile;
  for my $sampleIdx (0...($numSSRSamples-1)) {
    if ($samplesPerLine == 2) {
      $ssrSample = $ssrSamples[$sampleIdx];
      $ssrSample =~ /^((?:-)?\d+)\s+((?:-)?\d+)?/;
      if ($sampleIdx % 2 == 0) {
        $firstSample = $1;
      } else {
        $secondSample = $1;
      }
      $outFile[$sampleIdx/2] = "$firstSample $secondSample \n";
    } elsif ($linesPerSample == 2) {
      $ssrSample = $ssrSamples[$sampleIdx];
      $ssrSample =~ /([^\s]+)\s+([^\s]+)/;
      ($firstSample,$secondSample) = ($1,$2);
      $outFile[$sampleIdx*2] = "$firstSample \n";
      $outFile[$sampleIdx*2+1] = "$secondSample \n";
    } else {
      #in this case, they are not samples, so much as real/imag components
      $ssrSample = $ssrSamples[$sampleIdx];
      if ($complex) {
        $ssrSample =~ /([^\s]+)\s+([^\s]+)/;
        ($real,$imag) = ($1,$2);
        $outFile[$sampleIdx] = "$real $imag \n";
      } else {
        $ssrSample =~ /([^\s]+)/;
        $real = $1;
        $outFile[$sampleIdx] = "$real \n";
      }
    }
  }

  # newly re-arranged lines to result file
  print $fileH @outFile;
  close($fileH);
}
