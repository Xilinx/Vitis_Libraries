#/bin/bash

TEST_DIR=./out_test
STAT=./statistics.rpt

if [ -d $TEST_DIR ]; then
  rm $TEST_DIR -rf
fi

if [ -f $STAT ]; then
  rm $STAT -rf
fi

source set_env.sh

PYTHON=python3
PYTEST=./sw/python/run_test.py
PYCHECK=./sw/python/check_process.py
PAR=8
SUBMIT=bsub -cwd `pwd` -q medium -R "select[(os== lin && type == X86_64 && (osdistro == rhel || osdistro == centos) && (osver == ws6 || osver== ws7))] rusage[mem=16000]" 

$SUBMIT $PYTHON $PYTEST --operator amax amin asum axpy --parallel $PAR --id 0  --csim &
$SUBMIT $PYTHON $PYTEST --operator copy dot nrm2 scal swap --parallel $PAR  --id 1 --csim &
$SUBMIT $PYTHON $PYTEST --operator gemv trmvLo trmvUp --parallel $PAR  --id 2 --csim &
$SUBMIT $PYTHON $PYTEST --operator gbmv sbmvLo sbmvUp tbmvLo tbmvUp --parallel $PAR  --id 3 --csim &
$SUBMIT $PYTHON $PYTEST --operator symvLo symvUp spmvUp spmvLo tpmvLo tpmvUp --parallel $PAR --csim  --id 4&
$PYTHON $PYCHECK --name "statistics" --ext "rpt" --number 5

if [ -f $STAT ]; then
  cat $STAT
fi
