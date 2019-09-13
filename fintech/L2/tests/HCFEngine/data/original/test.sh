#!/bin/bash

#set -e

FILE=$1

filename=$(basename -- "$FILE")
filename="${filename%.*}"

LOGFILE="results/$filename".log
CSVFILE="results/$filename"_results.csv
TMPFILE=tmp.txt
dw=0.5
w_max=200
tol=0.001
EXE="./hcf_prototype.exe"

function extract()
{
    TMP=$(echo "$1" | cut -d' ' -f$2)
    X=$(echo $TMP | cut -d'=' -f2)
    echo $X
}

rm -f $LOGFILE
rm -f $CSVFILE

echo K, S0, V0, rho, vvol, vbar, T, r, kappa, tolerance, python call price, fpga call prince, difference, pass/fail > $CSVFILE
echo -e "\e[32m\e[1mRunning file $FILE\e[39m\e[0m"
while IFS= read -r line
do
    echo $line > $TMPFILE

    #get the test parameters
    K=$(extract "$line" "1")
    S0=$(extract "$line" "2")
    V0=$(extract "$line" "3")
    rho=$(extract "$line" "4")
    vvol=$(extract "$line" "5")
    vbar=$(extract "$line" "6") 
    T=$(extract "$line" "7")
    r=$(extract "$line" "8")
    kappa=$(extract "$line" "9")

    #python model
    output=$(python Hest_CharFunc_Test.py -K$K -S$S0 -V$V0 -R$rho -T$T -r$r -k$kappa -d$dw -x$w_max -m$vvol -l$vbar)
    pcall=$(echo $output | cut -d" " -f2)
    pcall=$(echo $pcall | cut -d")" -f1)
    pcall=$(printf '%.12f' $pcall)

    #fpga
    output=$($EXE -f$TMPFILE -d$dw -m$w_max -t$tol -v)
    fcall=$(echo $output | grep -o 'FPGA Call price = [^ ]\+')
    fcall=$(echo $fcall | cut -d"=" -f2)
    fcall=$(printf '%.12f' $fcall)

    #check the result
    res=FAIL
    diff=$(echo "$pcall - $fcall" | bc -l)
    if (( $(echo "$diff > 0" | bc -l) )); then
        if (( $(echo "$diff <= $tol" | bc -l) )); then
            res=PASS
        fi
    else
        if (( $(echo "$diff >= -$tol" | bc -l) )); then
            res=PASS
        fi
    fi

    #results
    echo $K, $S0, $V0, $rho, $vvol, $vbar, $T, $r, $kappa, $tol, $pcall, $fcall, $diff, $res >> $CSVFILE
    if [ $res == PASS ]; then
        echo $line ..... PASS >> $LOGFILE
        echo $line ..... PASS
    else
        echo $line ..... FAIL >> $LOGFILE
        echo "    pcall = $pcall" >> $LOGFILE
        echo "    fcall = $fcall" >> $LOGFILE
        echo "    diff  = $diff (tolerance = $tol)" >> $LOGFILE

        echo -e "\e[39m$line ..... \e[91mFAIL"
        echo -e "\e[36m    pcall = $pcall"
        echo "    fcall = $fcall"
        echo -e "    diff  = $diff (tolerance = $tol)\e[39m"
    fi

done < "$FILE"
