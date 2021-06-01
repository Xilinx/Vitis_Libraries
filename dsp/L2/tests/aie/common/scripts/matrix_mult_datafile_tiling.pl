#!/usr/bin/perl -w


use strict;
use warnings;
use Cwd;
use Cwd 'chdir';
use Getopt::Long;
use File::Basename;

use Term::ReadLine;

# TODO: accept STDIN as well as inFile. 

my $usage = "
This script will tile/untile an input text file with each sample on a newline.
use it thus:
matrix_mult_datafile_tiling.pl -f data/inputA.txt -tileRow 2 -tileCol 4 -r 16 -c 32
    The above will output a data/inputAtiled.txt file assuming that 16x32 row major matrix input. Will tile with 2x4 pattern.
options:
    -f|--file|--inFile=s   => input filepath containing input matrix of size r_c. mandatory.
    -m|--mtile|--tileRow=i => tileRows (M) dimension for AIE API mmult scheme
    -n|--ntile|--tileCol=i => tileCols (N) dimension for AIE API mmult scheme
    -r|--inRow=i           => Actual number of rows for InMatrix
    -c|--inCol=i           => Actual number of cols for InMatrix
    [-o|--outFile=s]       => Optional. output filepath (default is inFilePath/inFileName_[un]tiled.<inFileExt>)
    [-i|--inplace]         => Optional. the file contents of inFile are overwritten instead of default behaviour.
    [--colMajor]           => Optional. Specifies that the InMatrx is stored colMajor. Output will be tiled&rowMajor.
    [--untile]             => Optional. the input matrix is un-tiled. Works with other options, ie if colMajor present output will be stored ColumnMajor
    [-h|--help]            => Optional. prints this usage
    [-v|--verbose]         => Optional. additional logging

";


my $inFile = "";
my $outFile = "";
my $tileRow = "";
my $tileCol = "";
my $inRow = "";
my $inCol = "";
my $inplace = 0;
my $verbose = 0;
my $untile  = 0;
my $colMajor = 0;
my $help = 0;

GetOptions (
            "f|file|inFile=s"   => \$inFile,  # string
            "o|outFile=s"       => \$outFile,  # string
            "m|mtile|tileRow=i" => \$tileRow, # numeric
            "n|ntile|tileCol=i" => \$tileCol, # numeric
            "r|inRow=i"         => \$inRow,
            "c|inCol=i"         => \$inCol,
            "i|inplace"         => \$inplace,
            "untile"            => \$untile,
            "colMajor"          => \$colMajor,
            "h|help"            => \$help,
            "v|verbose"         => \$verbose) # flag
or die("Error in command line arguments\n");

if ( $help ) { 
    die "$usage";
}

# TODO: command line arguments for tile / untile and inplace / filename_tiled.txt

# Handle mandatory arguments
if ( $inFile eq "" ) { 
    die "ERROR: Provide command line argument for inFile. Use -h for usage. ";
}

if ( $tileRow eq "" ) { 
    die "ERROR: Provide command line argument for tileRow. Use -h for usage. ";
}
if ( $tileCol eq "" ) { 
    die "ERROR: Provide command line argument for tileCol. Use -h for usage. ";
}

if ( $inRow eq "" ) { 
    die "ERROR: Provide command line argument for inRow. Use -h for usage. ";
}

if ( $inCol eq "" ) { 
    die "ERROR: Provide command line argument for inCol. Use -h for usage. ";
}

# Handle verbose
if ( $verbose ) { 
    if ( $inFile ne "" ) { 
        print "inFile is $inFile. \n";
    }
    if ( $tileRow ne "" ) { 
        print "tileRow is $tileRow. \n";
    }
    if ( $tileCol ne "" ) { 
        print "tileCol is $tileCol. \n";
    }
    if ( $inRow ne "" ) { 
        print "inRow is $inRow. \n";
    }
    if ( $inCol ne "" ) { 
        print "inCol is $inCol. \n";
    }

    if  ($inplace) { 
        print "inplace is enabled\n";
    }
    if  ($colMajor) { 
        print "colMajor is enabled\n";
    }
    if  ($untile) { 
        print "untile is enabled\n";
    }
    if  ($help) { 
        print "help is enabled\n";
    }
    if  ($verbose) { 
        print "verbose is enabled\n";
    }
    if ($outFile ne "" ) { 
        print "outFile is $outFile. \n";
    }
}

(my $inFileName, my $inFileDir, my $inFileExt) = fileparse($inFile, '\..*');

my $outFileName;my $outFileDir;my $outFileExt;
if ($outFile ne "" ) { 
    ($outFileName, $outFileDir, $outFileExt) = fileparse($outFile, '\..*');
} else { 
    if ($inplace) {
        
        $outFileName = $inFileName;
        $outFileDir = $inFileDir;
        $outFileExt = $inFileExt;

    } else {
    my $un = "";
    if ($untile) { $un = "un"; }
    $outFileName = "${inFileName}_${un}tiled";
    $outFileDir = $inFileDir;
    $outFileExt = $inFileExt;
    }
    #print "$outFileName  : $outFileDir  :  $outFileExt \n" ; 
}
my $resOutFile = "${outFileDir}${outFileName}${outFileExt}";

print "OutFile is $resOutFile\n";
#(my $name,my $dir,my $ext) = fileparse($inFile);

#print "$inFileName : $inFileDir  : $inFileExt \n";
print "Reading $inFile. \n";

open(my $inFileh, "<" , $inFile)
    or die "Can't open < $inFile";

#my $line = readline($inFileh);
#print($line);
#$line = readline($inFileh);
#print($line);
my @inText;
while(<$inFileh>) { 
    chomp;
    push @inText, $_;
}

close($inFileh)
    or die "couldn't close inFileh $inFileh";

print "Finished reading file\n";

#print join "\n", @inText;

#print "$inText[1]\n";

# Now we print out the array in shuffled order. 

#print "$#inText\n";

my @duplicateText = @inText;
my @transText = @inText;
# fill with dummy data basically
my @indices = @inText;
my @transIndices = @inText;
#$((((($i-1)/($AB*$K))*$K + (($i-1)%$K))*$AB + ((($i-1)/$K) % $AB) +1 ))
#inRow
#inCol
#
#tileRow
#tileCol

#res=$(( (( (($i-1)/($AB*$M))*($AB*$M) + ((($i-1)/$N) % $M)) * $AB) +  (($i-1) % $N) + ((((($i-1)/($N*$M)) * $N) %  $AB)  + 1 ) ))
#open(my $outFileh, ">" , $resOutFile)
#    or die "Can't open > $resOutFile";

# TODO: ColumnMajor, Untiling
print "Shuffling indicies\n";
my @iIter = (0..$#inText);
for my $i (@iIter){ 
    my $newIndex;
    my $transposeIndex;
    {
        use integer;
        #print "(($i*$inRow) % ($inCol*$inRow)) + (($i/$inCol) % $inRow) + (($i/($inCol*$inRow))*($inCol*$inRow))\n ";
        $transposeIndex = (($i*$inRow) % ($inCol*$inRow)) + (($i/$inCol) % $inRow) + (($i/($inCol*$inRow))*($inCol*$inRow));
        # force everything to be integer arithmetic
                    # Coarse grain - increments of row of tile        Which tile in row of tiles                          which chunk of 4 samples within tile row    fine-grained within a chunk index
        $newIndex = (( $i/( $inCol * $tileRow ))*($inCol*$tileRow)) + ((($i/($tileRow*$tileCol)) * $tileCol) % $inCol) + ((( $i/$tileCol ) % $tileRow) * $inCol) + ($i % $tileCol);
    }
    #print "$i ($newIndex) => $transposeIndex \n";
    #print "$transposeIndex $transposeIndex\n";

    if ($untile) {
        $indices[$newIndex] = $i;
    } else { 
        $indices[$i] = $newIndex;
    }
    #if ($colMajor) {
        $transIndices[$i] = $transposeIndex;
    #}
    #print (int $i/( $inCol * $tileCol )); 
    #print "$newIndex \n";
}

print "Writing $resOutFile. \n";
open(my $outFileh, ">" , $resOutFile)
    or die "Can't open > $resOutFile";
#my @iIter = (0..$#inText/8);
for my $i (@iIter){ 
    #if ($colMajor) {
    #    $duplicateText[$i] = $inText[$transIndices[$indices[$i]]];
    #} else {
        $duplicateText[$i] = "$inText[$indices[$i]]";
    #}
}
for my $i (@iIter){ 
    if ($untile) { 
        $transText[$i] = "$duplicateText[$transIndices[$i]]";
    } else { 
        $transText[$i] = "$inText[$transIndices[$indices[$i]]]";
    }
    if ($colMajor) {
        print $outFileh "$transText[$i] \n";
    } else {
        print $outFileh "$duplicateText[$i] \n";
    }
    if ($verbose) {
        print "$i = $indices[$i] = $transIndices[$i]      \t \t ";
        print "$inText[$i] => $duplicateText[$i] => $transText[$i] \n";
    }
    #print "$i $i\n";
}

close($outFileh)
    or die "couldn't close outFileh $outFileh";


print "Finished writing $resOutFile.\nEnd of script.\n";