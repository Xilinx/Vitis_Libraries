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
matrix_mult_datafile_partition.pl -f data/inputA.txt -tileRow 2 -tileCol 4 -r 16 -c 32
    The above will output a data/inputAtiled.txt file assuming that 16x32 row major matrix input. Will tile with 2x4 pattern.
options:
    -f|--file|--inFile=s       => input filepath containing input matrix of size r_c. mandatory.
    -m|--mtile|--tileRow=i     => tileRows (M) dimension for AIE API mmult scheme
    -n|--ntile|--tileCol=i     => tileCols (N) dimension for AIE API mmult scheme
    -r|--inRow=i               => Actual number of rows for InMatrix
    -c|--inCol=i               => Actual number of cols for InMatrix
    -p|--partition|--cascLen=i => Number of partitions / Cascade length for InMatrix
    [--splitRows]              => Optional. Specify if input data is to be partitioned over rows. Default behaviour assumes split over columns. 
    [--isTiled]                => Optional. Specify if input data is already tiled. Default behaviour assumes it is not tiled. 
    [-o|--outFile=s]           => Optional. output filepath (default is inFilePath/inFileName_<casc_index>.<inFileExt>)
    [--colMajor]               => Optional. Specifies that the InMatrx is stored colMajor. Output will be tiled&rowMajor.
    [--untile]                 => Optional. the input matrix is un-tiled. Works with other options, ie if colMajor present output will be stored ColumnMajor
    [-h|--help]                => Optional. prints this usage
    [-v|--verbose]             => Optional. additional logging

";


my $inFile = "";
my $outFile = "";
my $tileRow = "";
my $tileCol = "";
my $inRow = "";
my $inCol = "";
my $cascLen = "";
my $splitRows  = 0;
my $verbose = 0;
my $untile  = 0;
my $isTiled  = 0;
my $colMajor = 0;
my $help = 0;

GetOptions (
            "f|file|inFile=s"       => \$inFile,  # string
            "o|outFile=s"           => \$outFile,  # string
            "m|mtile|tileRow=i"     => \$tileRow, # numeric
            "n|ntile|tileCol=i"     => \$tileCol, # numeric
            "r|inRow=i"             => \$inRow,
            "c|inCol=i"             => \$inCol,
            "p|partition|cascLen=i" => \$cascLen,
            "splitRows"             => \$splitRows,
            "untile"                => \$untile,
            "isTiled"                => \$isTiled,
            "colMajor"              => \$colMajor,
            "h|help"                => \$help,
            "v|verbose"             => \$verbose) # flag
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
if ( $cascLen eq "" ) { 
    die "ERROR: Provide command line argument for cascLen. Use -h for usage. ";
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
    $outFileName = "${inFileName}_";
    $outFileDir = $inFileDir;
    $outFileExt = $inFileExt;
    #print "$outFileName  : $outFileDir  :  $outFileExt \n" ; 
}
my $resOutFile = "${outFileDir}${outFileName}${outFileExt}";

print "Reading $inFile. \n";

open(my $inFileh, "<" , $inFile)
    or die "Can't open < $inFile";

my @inText;
while(<$inFileh>) { 
    chomp;
    push @inText, $_;
}

close($inFileh)
    or die "couldn't close inFileh $inFileh";

print "Finished reading file\n";

# todo, deal with cascades that don't evenly divide
# todo, deal with tiled data.
# todo, deal with column major.
my @duplicateText = @inText;
my $colsPerCasc;
my $rowsPerCasc;
if ( $splitRows ) {
    $rowsPerCasc = $inRow / $cascLen ; 
    $colsPerCasc = $inCol;
} else { 
    $colsPerCasc = $inCol / $cascLen ; 
    $rowsPerCasc = $inRow;
}
my @cascades = (0...($cascLen - 1));
my @columns = (0...($colsPerCasc - 1));
my @rows = (0...($rowsPerCasc - 1));
my @batches = (0...((($#inText + 1) / ( $inRow * $inCol ))) - 1);

my $colElementDist;
my $colElementDistCasc;
my $rowElementDist;
my $rowElementDistCasc;
if ( $colMajor ) {
    $rowElementDist = 1;
    $rowElementDistCasc = 1;
    $colElementDist = $inRow;
    $colElementDistCasc = $rowsPerCasc;
} else { 
    $rowElementDist = $inCol;
    $rowElementDistCasc = $colsPerCasc;
    $colElementDist = 1;
    $colElementDistCasc = 1;
}

if ( $verbose ) { 
    print "columns: ",join(", ", @columns), "\n";
    print "rows: ",join(", ", @rows), "\n";
    print "batches: ",join(", ", @batches), "\n";
}


my @outFileh;
my @resOutFiles;
print "writing output files\n";
for my $file (@cascades) {
    $resOutFiles[$file] = "${outFileDir}${outFileName}${file}${outFileExt}";
    print "$resOutFiles[$file] \n";
    open($outFileh[$file], ">", $resOutFiles[$file])
       or die "cannot open $resOutFiles[$file]: $!";
}


my $currentFile;
my $inTextIndex;
my $colIndex;
my $rowIndex;
my @outputArray;
for my $batch (@batches){
    if ( $colMajor ) { 
        for my $col (@columns){ 
            for my $row (@rows){
                for my $cascI ((0...($cascLen - 1))) {
                    $currentFile = $outFileh[$cascI];
                    if ( $splitRows ) { 
                        $colIndex = $col;
                        $rowIndex = ( $row + ( $cascI * $rowsPerCasc ) );
                    } else {
                        $colIndex = ( $col + ( $cascI * $colsPerCasc ) );
                        $rowIndex = $row;
                    }
                    $inTextIndex = (($batch * $inRow * $inCol) + ($rowIndex * $rowElementDist) + ( $colIndex * $colElementDist ));
                    print $currentFile "$inText[$inTextIndex]\n";
                    if ( $verbose )  { 
                        print "inText[$inTextIndex] = $inText[$inTextIndex]\n";
                    }
                }
            }
        }

    } else { 
        for my $row (@rows){
            for my $col (@columns){ 
                for my $cascI ((0...($cascLen - 1))) {
                    $currentFile = $outFileh[$cascI];
                    if ( $splitRows ) { 
                        $colIndex = $col;
                        $rowIndex = ( $row + ( $cascI * $rowsPerCasc ) );
                    } else {
                        $colIndex = ( $col + ( $cascI * $colsPerCasc ) );
                        $rowIndex = $row;
                    }
                    $inTextIndex = (($batch * $inRow * $inCol) + ($rowIndex * $rowElementDist) + ( $colIndex * $colElementDist ));
                    print $currentFile "$inText[$inTextIndex]\n";
                    if ( $verbose )  { 
                        print "inText[$inTextIndex] = $inText[$inTextIndex]\n";
                    }
                }
            }
        }
    }
}



for my $file (@cascades) {
    for my $i (@{$outputArray[$file]}) { 
        print "$i \n";
    }
    close($outFileh[$file])
        or die "couldn't close outFileh $outFileh[$file]: $!";
}

print "Finished writing @resOutFiles .\nEnd of script.\n";
